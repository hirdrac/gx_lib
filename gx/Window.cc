//
// gx/Window.cc
// Copyright (C) 2021 Richard Bradley
//

#include "Window.hh"
#include "OpenGLRenderer.hh"
#include "Logger.hh"
#include "System.hh"
//#include "Print.hh"
#include <algorithm>
#include <chrono>
#include <cassert>

#include <GLFW/glfw3.h>


namespace {
  // **** Time global & support functions ****
  using clock = std::chrono::steady_clock;
  clock::time_point startTime;

  inline void initStartTime()
  {
    if (startTime.time_since_epoch().count() == 0) {
      startTime = clock::now();
    }
  }

  inline int64_t usecSinceStart()
  {
    return std::chrono::duration_cast<std::chrono::microseconds>(
      clock::now() - startTime).count();
  }

  // **** Helper Functions ****
  constexpr int glfwBool(bool val) { return val ? GLFW_TRUE : GLFW_FALSE; }

  constexpr int cursorInputModeVal(gx::MouseModeEnum mode)
  {
    switch (mode) {
      case gx::MOUSE_NORMAL:  return GLFW_CURSOR_NORMAL;
      case gx::MOUSE_HIDE:    return GLFW_CURSOR_HIDDEN;
      case gx::MOUSE_DISABLE: return GLFW_CURSOR_DISABLED;
      default:                return -1;
    }
  }
}


gx::Window::~Window()
{
  if (_renderer) {
    assert(isMainThread());
    glfwHideWindow(_renderer->window());
    // window destroyed in Renderer destructor
  }
}

void gx::Window::setTitle(std::string_view title)
{
  _title = title;
  if (_renderer) {
    assert(isMainThread());
    glfwSetWindowTitle(_renderer->window(), _title.c_str());
  }
}

void gx::Window::setSize(int width, int height, bool fullScreen)
{
  if (_renderer) {
    assert(isMainThread());
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
    if (!_sizeSet) { showWindow(win); }
  } else {
    _width = width;
    _height = height;
  }

  _sizeSet = true;
  _fullScreen = fullScreen;
}

void gx::Window::setMouseMode(MouseModeEnum mode)
{
  int val = cursorInputModeVal(mode);
  if (val < 0) {
    LOG_ERROR("setMouseMode(): invalid mode ", mode);
    return;
  }

  _mouseMode = mode;
  if (_renderer) {
    assert(isMainThread());
    glfwSetInputMode(_renderer->window(), GLFW_CURSOR, val);
  }
}

void gx::Window::setMousePos(float x, float y)
{
  if (_renderer) {
    assert(isMainThread());
    glfwSetCursorPos(_renderer->window(), double(x), double(y));
  }
}

bool gx::Window::open(int flags)
{
  assert(isMainThread());
  if (!initGLFW()) {
    return false;
  }

  const bool decorated = flags & (WINDOW_DECORATED | WINDOW_RESIZABLE);
  const bool resizable = flags & WINDOW_RESIZABLE;
  const bool doubleBuffer = true;
  const bool fixedAspectRatio = false;
  const bool debug = flags & WINDOW_DEBUG;

  initStartTime();
  auto ren = std::make_shared<OpenGLRenderer>();
  // TODO - replace with factory constructor when multiple renderers
  //   are supported

  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_DECORATED, glfwBool(decorated));
  glfwWindowHint(GLFW_RESIZABLE, glfwBool(resizable));
  //glfwWindowHint(GLFW_SAMPLES, 0);
  glfwWindowHint(GLFW_DOUBLEBUFFER, glfwBool(doubleBuffer));
  glfwWindowHint(GLFW_VISIBLE, glfwBool(false));
  //glfwWindowHint(GLFW_FOCUSED, glfwBool(false));
  //glfwWindowHint(GLFW_FOCUS_ON_SHOW, glfwBool(false));
  ren->setWindowHints(debug);

  int width = _width;
  int height = _height;
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);

  // make sure video mode doesn't change for fullscreen
  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

  _fsWidth = mode->width;
  _fsHeight = mode->height;
  if (_fullScreen && (width <= 0 || height <= 0)) {
    width = _fsWidth;
    height = _fsHeight;
  } else if (!_sizeSet) {
    width = 256;
    height = 256;
  }

  GLFWwindow* win = glfwCreateWindow(
    width, height, _title.c_str(), _fullScreen ? monitor : nullptr, nullptr);
  if (!win) {
    LOG_ERROR("glfwCreateWindow() failed");
    return false;
  }

  glfwGetFramebufferSize(win, &_width, &_height);
  //println("new window size: ", _width, " x ", _height);
  bool status = ren->init(win);
  if (!status) {
    return false;
  }

  _renderer = std::move(ren);

  glfwSetWindowUserPointer(win, this);
  if (fixedAspectRatio) {
    glfwSetWindowAspectRatio(win, width, height);
  } else {
    glfwSetWindowAspectRatio(win, GLFW_DONT_CARE, GLFW_DONT_CARE);
  }

  glfwSetInputMode(win, GLFW_CURSOR, cursorInputModeVal(_mouseMode));
  if (resizable) {
    glfwSetWindowSizeLimits(win, _minWidth, _minHeight, _maxWidth, _maxHeight);
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

void gx::Window::showWindow(GLFWwindow* w)
{
  if (!_fullScreen) {
    // center window initially
    // FIXME - doesn't account for decoration size
    glfwSetWindowPos(w, (_fsWidth - _width) / 2, (_fsHeight - _height) / 2);
  }

  glfwShowWindow(w);
  // set initial mouse event state
  // (initial button state not supported by GLFW)
  updateMouseState(w);
}

void gx::Window::updateMouseState(GLFWwindow* w)
{
  double mx = 0, my = 0;
  glfwGetCursorPos(w, &mx, &my);
  _mouseX = mx;
  _mouseY = my;
  _mouseIn = (glfwGetWindowAttrib(w, GLFW_HOVERED) != 0);
}

int gx::Window::pollEvents()
{
  assert(isMainThread());

  // reset event state
  _events = 0;
  _removedEvents = 0;
  _scrollX = 0;
  _scrollY = 0;
  _chars.clear();
  for (auto i = _keyStates.begin(); i != _keyStates.end(); ) {
    if (i->pressed) {
      i->pressCount = 0;
      i->repeatCount = 0;
      ++i;
    } else {
      i = _keyStates.erase(i);
    }
  }

  // TODO - multi window support
  // - make function static
  // - reset event state for all windows
  // - return indicator for which window(s) received events?

  glfwPollEvents();
    // callbacks will set event values

  _lastPollTime = usecSinceStart();
  return _events;
}

bool gx::Window::keyPressed(int key) const
{
  auto itr = std::find_if(_keyStates.begin(), _keyStates.end(),
                          [key](const auto& ks){ return ks.key == key; });
  if (itr == _keyStates.end()) { return false; }
  return itr->pressed || (itr->pressCount > 0);
}

int gx::Window::keyPressCount(int key, bool includeRepeat) const
{
  auto itr = std::find_if(_keyStates.begin(), _keyStates.end(),
                          [key](const auto& ks){ return ks.key == key; });
  if (itr == _keyStates.end()) { return 0; }
  return itr->pressCount + (includeRepeat ? itr->repeatCount : 0);
}

// GLFW event callbacks
void gx::Window::closeCB(GLFWwindow* win)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    LOG_ERROR("unknown close window event");
    return;
  }

  //println("close event");
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_CLOSE;

  // tell GLFW not to close window
  glfwSetWindowShouldClose(win, GLFW_FALSE);
}

void gx::Window::sizeCB(GLFWwindow* win, int width, int height)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    LOG_ERROR("unknown size event");
    return;
  }

  //println("size event: ", width, ' ', height);
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_SIZE;
  e._width = width;
  e._height = height;
  e.updateMouseState(win);
}

void gx::Window::keyCB(
  GLFWwindow* win, int key, int scancode, int action, int mods)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    LOG_ERROR("unknown key event");
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
    ks.pressed = true;
  } else if (action == GLFW_RELEASE) {
    ks.pressed = false;
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

void gx::Window::charCB(GLFWwindow* win, unsigned int codepoint)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    LOG_ERROR("charmod event for unknown display");
    return;
  }

  //println("char event: ", codepoint);
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_CHAR;
  e._chars.push_back({codepoint, 0, 0, false});
}

void gx::Window::cursorEnterCB(GLFWwindow* win, int entered)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    LOG_ERROR("unknown cursor event");
    return;
  }

  //println("cursor enter event: ", entered);
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_MOUSE_ENTER;
  e._mouseIn = (entered != 0);
}

void gx::Window::cursorPosCB(GLFWwindow* win, double xpos, double ypos)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    LOG_ERROR("unknown cursor event");
    return;
  }

  //println("cursor pos event: ", xpos, ' ', ypos);
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_MOUSE_MOVE;
  e._mouseX = xpos;
  e._mouseY = ypos;
}

void gx::Window::mouseButtonCB(GLFWwindow* win, int button, int action, int mods)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    LOG_ERROR("unknown mouse button even");
    return;
  }

  auto& e = *static_cast<Window*>(ePtr);
  //println("mouse button event: ", button, ' ', action, ' ', mods);
  int b = 0; // bitfield value for button
  switch (button) {
    case GLFW_MOUSE_BUTTON_1:
      b = BUTTON1; e._events |= EVENT_MOUSE_BUTTON1; break;
    case GLFW_MOUSE_BUTTON_2:
      b = BUTTON2; e._events |= EVENT_MOUSE_BUTTON2; break;
    case GLFW_MOUSE_BUTTON_3:
      b = BUTTON3; e._events |= EVENT_MOUSE_BUTTON3; break;
    case GLFW_MOUSE_BUTTON_4:
      b = BUTTON4; e._events |= EVENT_MOUSE_BUTTON4; break;
    case GLFW_MOUSE_BUTTON_5:
      b = BUTTON5; e._events |= EVENT_MOUSE_BUTTON5; break;
    case GLFW_MOUSE_BUTTON_6:
      b = BUTTON6; e._events |= EVENT_MOUSE_BUTTON6; break;
    case GLFW_MOUSE_BUTTON_7:
      b = BUTTON7; e._events |= EVENT_MOUSE_BUTTON7; break;
    case GLFW_MOUSE_BUTTON_8:
      b = BUTTON8; e._events |= EVENT_MOUSE_BUTTON8; break;
    default:
      LOG_ERROR("unknown mouse button ", button);
      return;
  }

  e._mods = mods;
  if (action == GLFW_PRESS) {
    e._buttons |= b;
  } else if (action == GLFW_RELEASE) {
    e._buttons &= ~b;
  }
}

void gx::Window::scrollCB(GLFWwindow* win, double xoffset, double yoffset)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    LOG_ERROR("unknown mouse button event");
    return;
  }

  //println("scroll event: ", xoffset, ' ', yoffset);
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_MOUSE_SCROLL;
  e._scrollX += xoffset;
  e._scrollY += yoffset;
}

void gx::Window::iconifyCB(GLFWwindow* win, int iconified)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    LOG_ERROR("unknown iconify event");
    return;
  }

  //println("iconify event: ", iconified);
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_ICONIFY;
  e._iconified = iconified;
}

void gx::Window::focusCB(GLFWwindow* win, int focused)
{
  void* ePtr = glfwGetWindowUserPointer(win);
  if (!ePtr) {
    LOG_ERROR("unknown focus event");
    return;
  }

  //println("focus event: ", focused);
  auto& e = *static_cast<Window*>(ePtr);
  e._events |= EVENT_FOCUS;
  e._focused = focused;
}
