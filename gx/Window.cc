//
// gx/Window.cc
// Copyright (C) 2020 Richard Bradley
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
  std::chrono::high_resolution_clock::time_point startTime;

  inline void initStartTime()
  {
    int64_t t = std::chrono::duration_cast<std::chrono::microseconds>(
      startTime.time_since_epoch()).count();
    if (t == 0) {
      startTime = std::chrono::high_resolution_clock::now();
    }
  }

  inline int64_t timeSinceStart()
  {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(
      now - startTime).count();
  }

  // **** Helper Functions ****
  constexpr int glfwBool(bool val) { return val ? GLFW_TRUE : GLFW_FALSE; }

  int glfwCursorInputModeVal(gx::MouseModeEnum mode)
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
    glfwHideWindow(_renderer->window());
    // window destroyed in Renderer destructor
  }
}

void gx::Window::setTitle(std::string_view title)
{
  _title = title;
  if (_renderer) {
    glfwSetWindowTitle(_renderer->window(), _title.c_str());
  }
}

void gx::Window::setSize(int width, int height, bool fullScreen)
{
  if (_renderer) {
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
    glfwGetFramebufferSize(win, &_width, &_height);
    _fullScreen = fullScreen;
  } else {
    _width = width;
    _height = height;
    _fullScreen = fullScreen;
  }
}

void gx::Window::setMouseMode(MouseModeEnum mode)
{
  int val = glfwCursorInputModeVal(mode);
  if (val < 0) {
    LOG_ERROR("setMouseMode(): invalid mode ", mode);
    return;
  }

  _mouseMode = mode;
  if (_renderer) {
    glfwSetInputMode(_renderer->window(), GLFW_CURSOR, val);
  }
}

void gx::Window::setMousePos(float x, float y)
{
  if (_renderer) {
    glfwSetCursorPos(_renderer->window(), double(x), double(y));
  }
}

bool gx::Window::open()
{
  if (!initGLFW()) {
    return false;
  }

  initStartTime();
  auto ren = std::make_shared<OpenGLRenderer>();
  // TODO - replace with factory constructor when multiple renderers
  //   are supported

  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_DECORATED, glfwBool(_decorated));
  glfwWindowHint(GLFW_RESIZABLE, glfwBool(_resizable));
  glfwWindowHint(GLFW_DOUBLEBUFFER, glfwBool(_doubleBuffer));
  glfwWindowHint(GLFW_VISIBLE, glfwBool(false));
  //glfwWindowHint(GLFW_FOCUSED, glfwBool(false));
  //glfwWindowHint(GLFW_FOCUS_ON_SHOW, glfwBool(false));
  ren->setWindowHints(false); // no debug

  int width = _width;
  int height = _height;
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);

  // make sure video mode doesn't change for fullscreen
  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

  int fsWidth = mode->width;
  int fsHeight = mode->height;
  if (_fullScreen && (width <= 0 || height <= 0)) {
    width = fsWidth;
    height = fsHeight;
  }

  GLFWwindow* win = glfwCreateWindow(
    width, height, _title.c_str(), _fullScreen ? monitor : nullptr, nullptr);
  if (!win) {
    LOG_ERROR("glfwCreateWindow() failed");
    return false;
  }

  glfwSetWindowUserPointer(win, this);
  if (_fixedAspectRatio) {
    glfwSetWindowAspectRatio(win, width, height);
  } else {
    glfwSetWindowAspectRatio(win, GLFW_DONT_CARE, GLFW_DONT_CARE);
  }

  if (_resizable) {
    glfwSetWindowSizeLimits(win, _minWidth, _minHeight, _maxWidth, _maxHeight);
  }

  if (!_fullScreen) {
    // center window initially
    int sw = 0, sh = 0;
    glfwGetWindowSize(win, &sw, &sh);
    glfwSetWindowPos(win, (fsWidth - sw) / 2, (fsHeight - sh) / 2);
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

  glfwGetFramebufferSize(win, &_width, &_height);
  //println("new window size: ", _width, " x ", _height);
  bool status = ren->init(win);
  if (!status) {
    return false;
  }

  _renderer = std::move(ren);
  _maxTextureSize = _renderer->maxTextureSize();

  glfwSetInputMode(win, GLFW_CURSOR, glfwCursorInputModeVal( _mouseMode));
  glfwShowWindow(win);

  // set initial mouse event state
  // (initial button state not supported by GLFW)
  updateMouseState(win);
  return true;
}

void gx::Window::updateMouseState(GLFWwindow* w)
{
  double mx = 0, my = 0;
  glfwGetCursorPos(w, &mx, &my);
  _mouseX = mx;
  _mouseY = my;
  _mouseIn = (glfwGetWindowAttrib(w, GLFW_HOVERED) != 0);
}

void gx::Window::setBGColor(float r, float g, float b)
{
  assert(_renderer);
  _renderer->setBGColor(r,g,b);
}

void gx::Window::clearFrame()
{
  assert(_renderer);
  _renderer->clearFrame(_width, _height);
}

void gx::Window::draw(const DrawList& dl)
{
  assert(_renderer);
  _renderer->draw(dl, WHITE);
}

void gx::Window::draw(const DrawList& dl, const Color& modColor)
{
  assert(_renderer);
  _renderer->draw(dl, modColor);
}

void gx::Window::renderFrame()
{
  assert(_renderer);
  _renderer->renderFrame();
}

int gx::Window::pollEvents()
{
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

  _pollTime = timeSinceStart();
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
    if (key > 255) {
      e._events |= EVENT_CHAR;
      e._chars.push_back({0, int16_t(key), uint8_t(mods), false}); }
  } else if (action == GLFW_RELEASE) {
    ks.pressed = false;
  } else if (action == GLFW_REPEAT) {
    ++ks.repeatCount;
    if (key > 255) {
      e._events |= EVENT_CHAR;
      e._chars.push_back({0, int16_t(key), uint8_t(mods), true});
    }
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
