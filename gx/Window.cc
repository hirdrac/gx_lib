//
// gx/Window.cc
// Copyright (C) 2024 Richard Bradley
//

#include "Window.hh"
#include "OpenGLRenderer.hh"
#include "Logger.hh"
#include "System.hh"
#include "Assert.hh"
#include <algorithm>
#include <chrono>
#include <mutex>
#include <vector>
#include <GLFW/glfw3.h>
using namespace gx;

// NOTES:
// changing mouse pointer(cursor):
// GLFWcursor* ibeamCursor = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
// glfwSetCursor(win, ibeamCursor);
// glfwSetCursor(win, nullptr);
// glfwDestroyCursor(ibeamCursor);
//
// custom cursor:
// GLFWimage image;
// image.width = ...;
// image.height = ...;
// image.pixels = pointer to 32-bit RGBA value data (8-bits per color)
// GLFWcursor* custom = glfwCreateCursor(&image, xhot, yhot);
// glfwDestroyCursor(custom);


namespace {
  // assumed GLFW constant values
  static_assert(GLFW_DONT_CARE == -1);
  static_assert(GLFW_MOUSE_BUTTON_1 == 0);
  static_assert(GLFW_MOUSE_BUTTON_2 == 1);
  static_assert(GLFW_MOUSE_BUTTON_3 == 2);
  static_assert(GLFW_MOUSE_BUTTON_4 == 3);
  static_assert(GLFW_MOUSE_BUTTON_5 == 4);
  static_assert(GLFW_MOUSE_BUTTON_6 == 5);
  static_assert(GLFW_MOUSE_BUTTON_7 == 6);
  static_assert(GLFW_MOUSE_BUTTON_8 == 7);

  // **** Time global & support functions ****
  using clock = std::chrono::steady_clock;
  clock::time_point startTime;

  inline void initStartTime() {
    if (startTime.time_since_epoch().count() == 0) {
      startTime = clock::now();
    }
  }

  [[nodiscard]] inline int64_t usecSinceStart() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
      clock::now() - startTime).count();
  }

  // **** Helper Functions ****
  [[nodiscard]] constexpr int glfwBool(bool val) {
    return val ? GLFW_TRUE : GLFW_FALSE; }

  [[nodiscard]] constexpr int cursorInputModeVal(MouseModeEnum mode) {
    switch (mode) {
      case MOUSEMODE_NORMAL:  return GLFW_CURSOR_NORMAL;
      case MOUSEMODE_HIDE:    return GLFW_CURSOR_HIDDEN;
      case MOUSEMODE_DISABLE: return GLFW_CURSOR_DISABLED;
      default:                return -1;
    }
  }

  [[nodiscard]] GLFWcursor* getCursorInstance(MouseShapeEnum shape) {
    static GLFWcursor* ibeam = nullptr;
    static GLFWcursor* crosshair = nullptr;
      // cursors are freed with glfwTerminate()

    switch (shape) {
      case MOUSESHAPE_IBEAM:
        if (!ibeam) { ibeam = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR); }
        return ibeam;
      case MOUSESHAPE_CROSSHAIR:
        if (!crosshair) {
          crosshair = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR); }
        return crosshair;
      default: // MOUSESHAPE_ARROW
        return nullptr;
    }
  }

  // **** Window Instance Tracking ****
  std::vector<Window*> allWindows;
  std::mutex allWindowsMutex;
}


// **** Window class ****
int64_t Window::_lastPollTime = 0;

Window::Window()
{
  const std::lock_guard lg{allWindowsMutex};
  allWindows.push_back(this);
}

Window::~Window()
{
  {
    const std::lock_guard lg{allWindowsMutex};
    //std::erase(allWindows, this);  // C++20
    for (auto it = allWindows.begin(); it != allWindows.end(); ) {
      if (*it == this) {
        it = allWindows.erase(it);
      } else {
        ++it;
      }
    }
  }

  if (_renderer && glfwInitStatus()) {
    GX_ASSERT(isMainThread());
    glfwHideWindow(_renderer->window());
    // window destroyed in Renderer destructor
  }
}

void Window::setTitle(std::string_view title)
{
  _title = title;
  if (_renderer) {
    GX_ASSERT(isMainThread());
    glfwSetWindowTitle(_renderer->window(), _title.c_str());
  }
}

void Window::setSize(int width, int height, bool fullScreen)
{
  GX_ASSERT(width > 0 || fullScreen);
  GX_ASSERT(height > 0 || fullScreen);

  if (!fullScreen) {
    // update size limits if needed
    bool newLimits = false;
    if (width < _minWidth) { _minWidth = width; newLimits = true; }
    if (height < _minHeight) { _minHeight = height; newLimits = true; }
    if (_maxWidth > 0 && width > _maxWidth) {
      _maxWidth = width; newLimits = true; }
    if (_maxHeight > 0 && height > _maxHeight) {
      _maxHeight = height; newLimits = true; }
    if (newLimits) {
      setSizeLimits(_minWidth, _minHeight, _maxWidth, _maxHeight);
    }
  }

  if (_renderer) {
    GX_ASSERT(isMainThread());
    GLFWwindow* win = _renderer->window();
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int wx = 0, wy = 0;
    if (fullScreen) {
      if (width <= 0 || height <= 0) {
	width = mode->width;
	height = mode->height;
      }
    } else {
      wx = (mode->width - width) / 2;
      wy = (mode->height - height) / 2;
      monitor = nullptr;
    }

    glfwSetWindowMonitor(win, monitor, wx, wy, width, height, mode->refreshRate);
    _width = width;
    _height = height;
    _renderer->setFramebufferSize(width, height);
    if (!_sizeSet) {
      showWindow(win);
    } else {
      // ** WORK-AROUND **
      // (needed for version 3.3.4, recheck for newer versions)
      // extra restore/setWindow are to work around a bug where if the window
      // starts out fullscreen then is changed to windowed mode, it will
      // always be maximized
      glfwRestoreWindow(win);
      glfwSetWindowMonitor(
        win, monitor, wx, wy, width, height, mode->refreshRate);
    }

    if (_fixedAspectRatio) { glfwSetWindowAspectRatio(win, width, height); }
  } else {
    _width = width;
    _height = height;
  }

  _sizeSet = true;
  _fullScreen = fullScreen;
}

void Window::setSizeLimits(
  int minWidth, int minHeight, int maxWidth, int maxHeight)
{
  _minWidth = (minWidth < 0) ? -1 : minWidth;
  _minHeight = (minHeight < 0) ? -1 : minHeight;
  _maxWidth = (maxWidth < 0) ? -1 : maxWidth;
  _maxHeight = (maxHeight < 0) ? -1 : maxHeight;
  if (_renderer) {
    GX_ASSERT(isMainThread());
    glfwSetWindowSizeLimits(_renderer->window(), _minWidth, _minHeight,
                            _maxWidth, _maxHeight);
  }
}

void Window::setMouseMode(MouseModeEnum mode)
{
  const int val = cursorInputModeVal(mode);
  if (val < 0) {
    GX_LOG_ERROR("setMouseMode(): invalid mode ", mode);
    return;
  }

  _mouseMode = mode;
  if (_renderer) {
    GX_ASSERT(isMainThread());
    glfwSetInputMode(_renderer->window(), GLFW_CURSOR, val);
  }
}

void Window::setMouseShape(MouseShapeEnum shape)
{
  _mouseShape = shape;
  if (_renderer) {
    GX_ASSERT(isMainThread());
    glfwSetCursor(_renderer->window(), getCursorInstance(shape));
  }
}

void Window::setMousePos(Vec2 pos)
{
  _mousePt = pos;
  if (_renderer) {
    GX_ASSERT(isMainThread());
    glfwSetCursorPos(_renderer->window(), double(pos.x), double(pos.y));
  }
}

void Window::setSamples(int samples)
{
  _samples = std::max(0, samples);
  // FIXME: no effect if window has already been opened
}

bool Window::open(int flags)
{
  GX_ASSERT(isMainThread());
  if (!initGLFW()) {
    return false;
  }

  initStartTime();

  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);

  _fsWidth = mode->width;
  _fsHeight = mode->height;

  int width = 256;
  int height = 256;
  if (_sizeSet) {
    if (_fullScreen && (_width <= 0 || _height <= 0)) {
      width = _fsWidth;
      height = _fsHeight;
    } else {
      width = _width;
      height = _height;
    }
  }

  const bool decorated = flags & (WINDOW_DECORATED | WINDOW_RESIZABLE);
  const bool resizable = flags & WINDOW_RESIZABLE;
  const bool doubleBuffer = true;
  const bool fixedAspectRatio = flags & WINDOW_FIXED_ASPECT_RATIO;
  const bool debug = flags & WINDOW_DEBUG;

  // general window hints
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_DECORATED, glfwBool(decorated));
  glfwWindowHint(GLFW_RESIZABLE, glfwBool(resizable));
  glfwWindowHint(GLFW_VISIBLE, glfwBool(false));
  //glfwWindowHint(GLFW_FOCUSED, glfwBool(false));
  //glfwWindowHint(GLFW_FOCUS_ON_SHOW, glfwBool(false));

  // framebuffer hints
  glfwWindowHint(GLFW_SAMPLES, _samples);
  glfwWindowHint(GLFW_DOUBLEBUFFER, glfwBool(doubleBuffer));
  // make sure video mode doesn't change for fullscreen
  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

  // OpenGL specified window hints
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  //glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, glfwBool(debug));

  // use to force specific GL version for context
  //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_VERSION_MAJOR);
  //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_VERSION_MINOR);
  //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

  GLFWwindow* win = glfwCreateWindow(
    width, height, _title.c_str(), _fullScreen ? monitor : nullptr, nullptr);
  if (!win) {
    GX_LOG_ERROR("glfwCreateWindow() failed");
    return false;
  }

  auto ren = makeOpenGLRenderer(win);
  if (!ren || !ren->init(win)) { return false; }

  _width = ren->framebufferWidth();
  _height = ren->framebufferHeight();
  _renderer = std::move(ren);

  glfwSetWindowUserPointer(win, this);
  glfwSetInputMode(win, GLFW_CURSOR, cursorInputModeVal(_mouseMode));
  glfwSetCursor(win, getCursorInstance(_mouseShape));
  if (resizable) {
    if (flags & WINDOW_LIMIT_MIN_SIZE) {
      _minWidth = width;
      _minHeight = height;
    }
    if (flags & WINDOW_LIMIT_MAX_SIZE) {
      _maxWidth = width;
      _maxHeight = height;
    }
    glfwSetWindowSizeLimits(win, _minWidth, _minHeight, _maxWidth, _maxHeight);
    _fixedAspectRatio = fixedAspectRatio;
    if (fixedAspectRatio) {
      glfwSetWindowAspectRatio(win, width, height);
    } else {
      glfwSetWindowAspectRatio(win, GLFW_DONT_CARE, GLFW_DONT_CARE);
    }
  }

  _events = EVENT_SIZE; // always generate a resize event initially
  _removedEvents = 0;
  glfwSetWindowCloseCallback(win, closeCB);
  glfwSetFramebufferSizeCallback(win, sizeCB);
  glfwSetKeyCallback(win, keyCB);
  glfwSetCharCallback(win, charCB);
  glfwSetCursorEnterCallback(win, cursorEnterCB);
  glfwSetCursorPosCallback(win, cursorPosCB);
  glfwSetMouseButtonCallback(win, mouseButtonCB);
  glfwSetScrollCallback(win, scrollCB);
  glfwSetWindowIconifyCallback(win, iconifyCB);
  glfwSetWindowFocusCallback(win, focusCB);

  if (_sizeSet) { showWindow(win); }
  return true;
}

void Window::showWindow(GLFWwindow* w)
{
  if (!_fullScreen) {
    // center window initially
    // FIXME: doesn't account for decoration size
    glfwSetWindowPos(w, (_fsWidth - _width) / 2, (_fsHeight - _height) / 2);
  }

  glfwShowWindow(w);

  // unmaximize if window started out maximized
  // (glfwShowWindow() does this if window is too large to fit on screen)
  glfwRestoreWindow(w);

  // set initial mouse event state
  // (initial button state not supported by GLFW)
  updateMouseState(w);
}

void Window::updateMouseState(GLFWwindow* w)
{
  double mx = 0, my = 0;
  glfwGetCursorPos(w, &mx, &my);
  _mousePt.set(float(mx), float(my));
  _mouseIn = (glfwGetWindowAttrib(w, GLFW_HOVERED) != 0);
}

void Window::resetEventState()
{
  _events = 0;
  _removedEvents = 0;
  _scrollPt.set(0,0);
  _chars.clear();
  for (auto i = _keyStates.begin(); i != _keyStates.end(); ) {
    if (i->held) {
      i->pressCount = 0;
      i->repeatCount = 0;
      ++i;
    } else {
      i = _keyStates.erase(i);
    }
  }
}

void Window::finalizeEventState()
{
  // button event pressing
  if (_buttonsPress != 0 || _buttonsRelease != 0) {
    for (int b = 0; b < 8; ++b) {
      const int bVal = BUTTON1<<b;
      const int bEvent = EVENT_MOUSE_BUTTON1<<b;

      if (_buttonsPress & bVal) {
        _events |= bEvent;
      } else if (_buttonsRelease & bVal) {
        // button release event is delayed to next update if it happened in the
        // same event poll as the press event
        _events |= bEvent;
        _buttonsRelease &= ~bVal;
        _buttons &= ~bVal;
      }
    }

    _buttons |= _buttonsPress;
    _buttonsPress = 0;
  }
}

int Window::pollEvents()
{
  GX_ASSERT(isMainThread());
  int e = 0;

  {
    const std::lock_guard lg{allWindowsMutex};
    for (auto w : allWindows) {
      w->resetEventState();
    }

    glfwPollEvents();
      // callbacks will set event values

    for (auto w : allWindows) {
      w->finalizeEventState();
      e |= w->_events;
    }
  }

  _lastPollTime = usecSinceStart();
  return e;
}

bool Window::keyHeld(int key) const
{
  auto itr = std::find_if(_keyStates.begin(), _keyStates.end(),
                          [key](const auto& ks){ return ks.key == key; });
  if (itr == _keyStates.end()) { return false; }
  return itr->held || (itr->pressCount > 0);
}

int Window::keyPressCount(int key, bool includeRepeat) const
{
  auto itr = std::find_if(_keyStates.begin(), _keyStates.end(),
                          [key](const auto& ks){ return ks.key == key; });
  if (itr == _keyStates.end()) { return 0; }
  return itr->pressCount + (includeRepeat ? itr->repeatCount : 0);
}

// GLFW event callbacks
void Window::closeCB(GLFWwindow* win)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown close event");
    return;
  }

  //println("close event");
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_CLOSE;

  // tell GLFW not to close window
  glfwSetWindowShouldClose(win, GLFW_FALSE);
}

void Window::sizeCB(GLFWwindow* win, int width, int height)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown size event");
    return;
  }

  //println("size event: ", width, ' ', height);
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_SIZE;
  e._width = width;
  e._height = height;
  if (e._renderer) { e._renderer->setFramebufferSize(width, height); }
  e.updateMouseState(win);
}

void Window::keyCB(
  GLFWwindow* win, int key, int scancode, int action, int mods)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown key event");
    return;
  }

  //println("key event: ", key, ' ', scancode, ' ', action, ' ', mods);
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_KEY;
  e._mods = mods;

  auto itr = std::find_if(e._keyStates.begin(), e._keyStates.end(),
			  [key](const auto& ks){ return ks.key == key; });
  if (itr == e._keyStates.end()) {
    itr = e._keyStates.insert(e._keyStates.end(), {int16_t(key),0,0,false});
  }

  KeyState& ks = *itr;
  if (action == GLFW_PRESS) {
    ++ks.pressCount;
    ks.held = true;
    switch (key) {
      case KEY_LSHIFT:   case KEY_RSHIFT:   e._mods |= MOD_SHIFT; break;
      case KEY_LCONTROL: case KEY_RCONTROL: e._mods |= MOD_CONTROL; break;
      case KEY_LALT:     case KEY_RALT:     e._mods |= MOD_ALT; break;
      case KEY_LSUPER:   case KEY_RSUPER:   e._mods |= MOD_SUPER; break;
    }
  } else if (action == GLFW_RELEASE) {
    ks.held = false;
    switch (key) {
      case KEY_LSHIFT:   case KEY_RSHIFT:   e._mods &= ~MOD_SHIFT; break;
      case KEY_LCONTROL: case KEY_RCONTROL: e._mods &= ~MOD_CONTROL; break;
      case KEY_LALT:     case KEY_RALT:     e._mods &= ~MOD_ALT; break;
      case KEY_LSUPER:   case KEY_RSUPER:   e._mods &= ~MOD_SUPER; break;
    }
  } else if (action == GLFW_REPEAT) {
    ++ks.repeatCount;
  }

  if ((action == GLFW_PRESS || action == GLFW_REPEAT)
      && (key > 255 || mods & (MOD_CONTROL | MOD_ALT)))
  {
    // generate fake char events so control keys can be processed inline
    //   with actual char events
    //
    // ISSUES
    // - MOD_SUPER+key produces real char event so it's excluded here

    //println("gen char event: ", key, ' ', mods);
    e._events |= EVENT_CHAR;
    e._chars.push_back({0, int16_t(key), uint8_t(mods), action == GLFW_REPEAT});
  }
}

void Window::charCB(GLFWwindow* win, unsigned int codepoint)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown char event");
    return;
  }

  //println("char event: ", codepoint);
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_CHAR;
  e._chars.push_back({codepoint, 0, 0, false});
}

void Window::cursorEnterCB(GLFWwindow* win, int entered)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown cursor enter event");
    return;
  }

  //println("cursor enter event: ", entered);
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_MOUSE_ENTER;
  e._mouseIn = (entered != 0);
}

void Window::cursorPosCB(GLFWwindow* win, double xpos, double ypos)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown cursor pos event");
    return;
  }

  //println("cursor pos event: ", xpos, ' ', ypos);
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_MOUSE_MOVE;
  e._mousePt.set(float(xpos), float(ypos));
}

void Window::mouseButtonCB(GLFWwindow* win, int button, int action, int mods)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown mouse button even");
    return;
  }

  auto& e = *static_cast<Window*>(ePtr);
  //println("mouse button event: ", button, ' ', action, ' ', mods);
  if (button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_8) {
    GX_LOG_ERROR("unknown mouse button ", button);
    return;
  }

  // bitfield value for button
  const int bVal = BUTTON1<<button;

  e._mods = mods;
  if (action == GLFW_PRESS) {
    e._buttonsPress |= bVal;
    //println("button press:", int(b));
  } else if (action == GLFW_RELEASE) {
    e._buttonsRelease |= bVal;
    //println("button release:", int(b));
  }
}

void Window::scrollCB(GLFWwindow* win, double xoffset, double yoffset)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown scroll event");
    return;
  }

  //println("scroll event: ", xoffset, ' ', yoffset);
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_MOUSE_SCROLL;
  e._scrollPt.x += float(xoffset);
  e._scrollPt.y += float(yoffset);
}

void Window::iconifyCB(GLFWwindow* win, int iconified)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown iconify event");
    return;
  }

  //println("iconify event: ", iconified);
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_ICONIFY;
  e._iconified = iconified;
}

void Window::focusCB(GLFWwindow* win, int focused)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown focus event");
    return;
  }

  //println("focus event: ", focused);
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_FOCUS;
  e._focused = focused;
}
