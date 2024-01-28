//
// gx/Window.cc
// Copyright (C) 2024 Richard Bradley
//

#include "Window.hh"
#include "OpenGLRenderer.hh"
#include "Logger.hh"
#include "System.hh"
#include "Assert.hh"
#include "ThreadID.hh"
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
  std::vector<WindowImpl*> allImpls;
  std::mutex allImplsMutex;
}

static void closeCB(GLFWwindow* win);
static void sizeCB(GLFWwindow* win, int width, int height);
static void keyCB(GLFWwindow* win, int key, int scancode, int action, int mods);
static void charCB(GLFWwindow* win, unsigned int codepoint);
static void cursorEnterCB(GLFWwindow* win, int entered);
static void cursorPosCB(GLFWwindow* win, double xpos, double ypos);
static void mouseButtonCB(GLFWwindow* win, int button, int action, int mods);
static void scrollCB(GLFWwindow* win, double xoffset, double yoffset);
static void iconifyCB(GLFWwindow* win, int iconified);
static void focusCB(GLFWwindow* win, int focused);


// **** WindowImpl class ****
struct gx::WindowImpl
{
  WindowImpl()
  {
    const std::lock_guard lg{allImplsMutex};
    allImpls.push_back(this);
  }

  ~WindowImpl()
  {
    {
      const std::lock_guard lg{allImplsMutex};
      //std::erase(allImpls, this);  // C++20
      for (auto it = allImpls.begin(); it != allImpls.end(); ) {
        if (*it == this) {
          it = allImpls.erase(it);
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

  void setTitle(std::string_view title)
  {
    _title = title;
    if (_renderer) {
      GX_ASSERT(isMainThread());
      glfwSetWindowTitle(_renderer->window(), _title.c_str());
    }
  }

  void setSize(int width, int height, bool fullScreen)
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

      glfwSetWindowMonitor(
        win, monitor, wx, wy, width, height, mode->refreshRate);
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

  void setSizeLimits(int minWidth, int minHeight, int maxWidth, int maxHeight)
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

  void setMouseMode(MouseModeEnum mode)
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

  void setMouseShape(MouseShapeEnum shape)
  {
    _mouseShape = shape;
    if (_renderer) {
      GX_ASSERT(isMainThread());
      glfwSetCursor(_renderer->window(), getCursorInstance(shape));
    }
  }

  void setMousePos(Vec2 pos)
  {
    _mousePt = pos;
    if (_renderer) {
      GX_ASSERT(isMainThread());
      glfwSetCursorPos(_renderer->window(), double(pos.x), double(pos.y));
    }
  }

  void setSamples(int samples)
  {
    _samples = std::max(0, samples);
    // FIXME: no effect if window has already been opened
  }

  bool open(int flags)
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

  void showWindow(GLFWwindow* w)
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

  void updateMouseState(GLFWwindow* w)
  {
    double mx = 0, my = 0;
    glfwGetCursorPos(w, &mx, &my);
    _mousePt.set(float(mx), float(my));
    _mouseIn = (glfwGetWindowAttrib(w, GLFW_HOVERED) != 0);
  }

  void resetEventState()
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

  void finalizeEventState()
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

  std::unique_ptr<Renderer> _renderer;
  int _width = 0, _height = 0;
  int _fsWidth = 0, _fsHeight = 0;
  int _minWidth = -1, _minHeight = -1;
  int _maxWidth = -1, _maxHeight = -1;
  int _samples = 4; // for MSAA, 0 disables multi-sampling
  std::string _title;
  MouseModeEnum _mouseMode = MOUSEMODE_NORMAL;
  MouseShapeEnum _mouseShape = MOUSESHAPE_ARROW;
  bool _sizeSet = false;
  bool _fullScreen = false;
  bool _fixedAspectRatio = false;

  // event state
  int _events = 0, _removedEvents = 0;
  std::vector<KeyState> _keyStates;
  std::vector<CharInfo> _chars;
  Vec2 _mousePt{0,0}, _scrollPt{0,0};
  int _buttons = 0, _mods = 0;
  int _buttonsPress = 0, _buttonsRelease = 0;
  bool _mouseIn = false, _iconified = false, _focused = true;
};


// **** Window class ****
int64_t Window::_lastPollTime = 0;

Window::Window() : _impl{new WindowImpl} { }
Window::~Window() { }

void Window::setTitle(std::string_view title) {
  _impl->setTitle(title); }
void Window::setSize(int width, int height, bool fullScreen) {
  _impl->setSize(width, height, fullScreen); }
void Window::setSizeLimits(
  int minWidth, int minHeight, int maxWidth, int maxHeight) {
  _impl->setSizeLimits(minWidth, minHeight, maxWidth, maxHeight); }
void Window::setMouseMode(MouseModeEnum mode) {
  _impl->setMouseMode(mode); }
void Window::setMouseShape(MouseShapeEnum shape) {
  _impl->setMouseShape(shape); }
void Window::setMousePos(Vec2 pos) {
  _impl->setMousePos(pos); }
void Window::setSamples(int samples) {
  _impl->setSamples(samples); }
bool Window::open(int flags) {
  return _impl->open(flags); }
int Window::width() const {
  return _impl->_width; }
int Window::height() const {
  return _impl->_height; }
std::pair<int,int> Window::dimensions() const {
  return {_impl->_width, _impl->_height}; }
const std::string& Window::title() const {
  return _impl->_title; }
bool Window::fullScreen() const {
  return _impl->_fullScreen; }

int Window::pollEvents()
{
  GX_ASSERT(isMainThread());
  int e = 0;

  {
    const std::lock_guard lg{allImplsMutex};
    for (auto w : allImpls) {
      w->resetEventState();
    }

    glfwPollEvents();
      // callbacks will set event values

    for (auto w : allImpls) {
      w->finalizeEventState();
      e |= w->_events;
    }
  }

  _lastPollTime = usecSinceStart();
  return e;
}

int Window::events() const {
  return _impl->_events; }
int Window::allEvents() const {
  return _impl->_events | _impl->_removedEvents; }

void Window::removeEvent(int event_mask)
{
  int e = _impl->_events & event_mask;
  _impl->_removedEvents |= e;
  _impl->_events &= ~e;
}

bool Window::iconified() const {
  return _impl->_iconified; }
bool Window::focused() const {
  return _impl->_focused; }
MouseModeEnum Window::mouseMode() const {
  return _impl->_mouseMode; }
MouseShapeEnum Window::mouseShape() const {
  return _impl->_mouseShape; }
Vec2 Window::mousePt() const {
  return _impl->_mousePt; }
Vec2 Window::scrollPt() const {
  return _impl->_scrollPt; }
int Window::buttons() const {
  return _impl->_buttons; }
bool Window::mouseIn() const {
  return _impl->_mouseIn; }
bool Window::buttonPress(ButtonEnum button) const {
  return (_impl->_events & (button<<11)) && (_impl->_buttons & button); }
bool Window::buttonRelease(ButtonEnum button) const {
  return (_impl->_events & (button<<11)) && !(_impl->_buttons & button); }
bool Window::buttonDrag(int button_mask) const {
  return (_impl->_events & EVENT_MOUSE_MOVE)
    && ((_impl->_buttons & button_mask) == button_mask); }

bool Window::keyHeld(int key) const
{
  const auto& states = _impl->_keyStates;
  auto itr = std::find_if(states.begin(), states.end(),
                          [key](const auto& ks){ return ks.key == key; });
  if (itr == states.end()) { return false; }
  return itr->held || (itr->pressCount > 0);
}

int Window::keyPressCount(int key, bool includeRepeat) const
{
  const auto& states = _impl->_keyStates;
  auto itr = std::find_if(states.begin(), states.end(),
                          [key](const auto& ks){ return ks.key == key; });
  if (itr == states.end()) { return 0; }
  return itr->pressCount + (includeRepeat ? itr->repeatCount : 0);
}

const std::vector<KeyState>& Window::keyStates() const {
  return _impl->_keyStates; }
int Window::keyMods() const {
  return _impl->_mods; }
const std::vector<CharInfo>& Window::charData() const {
  return _impl->_chars; }

Renderer& Window::renderer()
{
  GX_ASSERT(_impl->_renderer != nullptr);
  return *(_impl->_renderer);
}

// GLFW event callbacks
static void closeCB(GLFWwindow* win)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown close event");
    return;
  }

  //println("close event");
  auto& e = *static_cast<WindowImpl*>(ePtr);
  e._events |= EVENT_CLOSE;

  // tell GLFW not to close window
  glfwSetWindowShouldClose(win, GLFW_FALSE);
}

static void sizeCB(GLFWwindow* win, int width, int height)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown size event");
    return;
  }

  //println("size event: ", width, ' ', height);
  auto& e = *static_cast<WindowImpl*>(ePtr);
  e._events |= EVENT_SIZE;
  e._width = width;
  e._height = height;
  if (e._renderer) { e._renderer->setFramebufferSize(width, height); }
  e.updateMouseState(win);
}

static void keyCB(GLFWwindow* win, int key, int scancode, int action, int mods)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown key event");
    return;
  }

  //println("key event: ", key, ' ', scancode, ' ', action, ' ', mods);
  auto& e = *static_cast<WindowImpl*>(ePtr);
  e._events |= EVENT_KEY;
  e._mods = mods;

  auto& states = e._keyStates;
  auto itr = std::find_if(states.begin(), states.end(),
			  [key](const auto& ks){ return ks.key == key; });
  if (itr == states.end()) {
    itr = states.insert(states.end(), {int16_t(key),0,0,false});
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

static void charCB(GLFWwindow* win, unsigned int codepoint)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown char event");
    return;
  }

  //println("char event: ", codepoint);
  auto& e = *static_cast<WindowImpl*>(ePtr);
  e._events |= EVENT_CHAR;
  e._chars.push_back({codepoint, 0, 0, false});
}

static void cursorEnterCB(GLFWwindow* win, int entered)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown cursor enter event");
    return;
  }

  //println("cursor enter event: ", entered);
  auto& e = *static_cast<WindowImpl*>(ePtr);
  e._events |= EVENT_MOUSE_ENTER;
  e._mouseIn = (entered != 0);
}

static void cursorPosCB(GLFWwindow* win, double xpos, double ypos)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown cursor pos event");
    return;
  }

  //println("cursor pos event: ", xpos, ' ', ypos);
  auto& e = *static_cast<WindowImpl*>(ePtr);
  e._events |= EVENT_MOUSE_MOVE;
  e._mousePt.set(float(xpos), float(ypos));
}

static void mouseButtonCB(GLFWwindow* win, int button, int action, int mods)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown mouse button even");
    return;
  }

  auto& e = *static_cast<WindowImpl*>(ePtr);
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

static void scrollCB(GLFWwindow* win, double xoffset, double yoffset)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown scroll event");
    return;
  }

  //println("scroll event: ", xoffset, ' ', yoffset);
  auto& e = *static_cast<WindowImpl*>(ePtr);
  e._events |= EVENT_MOUSE_SCROLL;
  e._scrollPt.x += float(xoffset);
  e._scrollPt.y += float(yoffset);
}

static void iconifyCB(GLFWwindow* win, int iconified)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown iconify event");
    return;
  }

  //println("iconify event: ", iconified);
  auto& e = *static_cast<WindowImpl*>(ePtr);
  e._events |= EVENT_ICONIFY;
  e._iconified = iconified;
}

static void focusCB(GLFWwindow* win, int focused)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    GX_LOG_ERROR("unknown focus event");
    return;
  }

  //println("focus event: ", focused);
  auto& e = *static_cast<WindowImpl*>(ePtr);
  e._events |= EVENT_FOCUS;
  e._focused = focused;
}
