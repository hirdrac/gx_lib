//
// gx/WindowGLFWImpl.cc
// Copyright (C) 2026 Richard Bradley
//

// TODO: mouseIn can get lost with resize on Windows - auto set mouseIn for
//   valid mouseMove events?
// TODO: check if mouseIn even works for Windows

#include "WindowGLFWImpl.hh"
#include "EventState.hh"
#include "OpenGLRenderer.hh"
#include "OpenGL.hh"
#include "Logger.hh"
#include "ThreadID.hh"
#include "GLFW.hh"
#include "Unicode.hh"
#include <algorithm>
#ifdef _WIN32
#  define GLFW_EXPOSE_NATIVE_WIN32
#  include <GLFW/glfw3native.h>
#  include <dwmapi.h>
#  ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#    define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#  endif
#endif
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

  // **** Helper Functions ****
  [[nodiscard]] constexpr int cursorInputModeVal(MouseModeEnum mode) {
    switch (mode) {
      case MOUSEMODE_NORMAL:  return GLFW_CURSOR_NORMAL;
      case MOUSEMODE_HIDE:    return GLFW_CURSOR_HIDDEN;
      case MOUSEMODE_DISABLE: return GLFW_CURSOR_DISABLED;
      default:                return -1;
    }
  }

  [[nodiscard]] GLFWcursor* getCursorInstance(MouseShapeEnum shape) {
    constexpr int cursorID[6] {
      GLFW_ARROW_CURSOR, GLFW_IBEAM_CURSOR, GLFW_CROSSHAIR_CURSOR,
      GLFW_HAND_CURSOR, GLFW_HRESIZE_CURSOR, GLFW_VRESIZE_CURSOR};
    static GLFWcursor* cursor[6] {};
      // cursors are freed with glfwTerminate()

    const int i = int(shape);
    if (i <= 0 || i > MOUSESHAPE_VRESIZE) {
      return nullptr; // MOUSESHAPE_ARROW
    }

    if (!cursor[i]) {
      cursor[i] = glfwCreateStandardCursor(cursorID[i]);
    }
    return cursor[i];
  }
}


// **** WindowImpl class ****
WindowImpl::WindowImpl()
{
  // initial event state
  _eventState.events = EVENT_SIZE;
  _eventState.focused = true;
}

WindowImpl::~WindowImpl()
{
  if (_window && glfwInitStatus()) {
    GX_ASSERT(isMainThread());
    glfwDestroyWindow(_window);
  }
}

void WindowImpl::setTitle(std::string_view title)
{
  _title = title;
  if (_renderer) {
    GX_ASSERT(isMainThread());
    glfwSetWindowTitle(_window, _title.c_str());
  }
}

void WindowImpl::setSize(int width, int height, bool fullScreen)
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
    //GLFWmonitor* monitor = glfwGetWindowMonitor(_window);
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int wx = 0, wy = 0;
    if (!fullScreen) {
      wx = (mode->width - width) / 2;
      wy = (mode->height - height) / 2;
      monitor = nullptr;
    } else if (width <= 0 || height <= 0) {
      width = mode->width;
      height = mode->height;
    }

    glfwSetWindowMonitor(
      _window, monitor, wx, wy, width, height, mode->refreshRate);
    _width = width;
    _height = height;
    _renderer->setFramebufferSize(width, height);
    _genSizeEvent = true;
    if (!_sizeSet) {
      showWindow();
    } else {
      // ** WORK-AROUND **
      // (needed for version 3.3.4, recheck for newer versions)
      // extra restore/setWindow are to work around a bug where if the window
      // starts out fullscreen then is changed to windowed mode, it will
      // always be maximized
      glfwRestoreWindow(_window);
      glfwSetWindowMonitor(
        _window, monitor, wx, wy, width, height, mode->refreshRate);
    }

    if (_fixedAspectRatio) { glfwSetWindowAspectRatio(_window, width, height); }
  } else {
    _width = width;
    _height = height;
  }

  _sizeSet = true;
  _fullScreen = fullScreen;
}


void WindowImpl::setSizeLimits(
  int minWidth, int minHeight, int maxWidth, int maxHeight)
{
  _minWidth = (minWidth < 0) ? -1 : minWidth;
  _minHeight = (minHeight < 0) ? -1 : minHeight;
  _maxWidth = (maxWidth < 0) ? -1 : maxWidth;
  _maxHeight = (maxHeight < 0) ? -1 : maxHeight;
  if (_renderer) {
    GX_ASSERT(isMainThread());
    glfwSetWindowSizeLimits(
      _window, _minWidth, _minHeight, _maxWidth, _maxHeight);
  }
}

void WindowImpl::setMouseMode(MouseModeEnum mode)
{
  const int val = cursorInputModeVal(mode);
  if (val < 0) {
    GX_LOG_ERROR("setMouseMode(): invalid mode ", mode);
    return;
  }

  _mouseMode = mode;
  if (_renderer) {
    GX_ASSERT(isMainThread());
    glfwSetInputMode(_window, GLFW_CURSOR, val);
  }
}

void WindowImpl::setMouseShape(MouseShapeEnum shape)
{
  _mouseShape = shape;
  if (_renderer) {
    GX_ASSERT(isMainThread());
    glfwSetCursor(_window, getCursorInstance(shape));
  }
}

void WindowImpl::setMousePos(Vec2 pos)
{
  _eventState.mousePt = pos;
  if (_renderer) {
    GX_ASSERT(isMainThread());
    glfwSetCursorPos(_window, double(pos.x), double(pos.y));
  }
}

void WindowImpl::setSamples(int samples)
{
  _samples = std::max(0, samples);
  // FIXME: no effect if window has already been opened
}

bool WindowImpl::open(int flags)
{
  GX_ASSERT(isMainThread());
  if (!initGLFW()) {
    return false;
  }

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
  if (_sizeSet && !_fullScreen) {
    glfwWindowHint(GLFW_POSITION_X, (_fsWidth - _width) / 2);
    glfwWindowHint(GLFW_POSITION_Y, (_fsHeight - _height) / 2);
  }

  //glfwWindowHint(GLFW_FOCUSED, glfwBool(false));
  //glfwWindowHint(GLFW_FOCUS_ON_SHOW, glfwBool(false));
  //glfwWindowHint(GLFW_WIN32_KEYBOARD_MENU, glfwBool(true));

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

#ifdef _WIN32
  // allow Windows to use dark mode for titlebar
  {
    HWND hwnd = glfwGetWin32Window(win);
    BOOL value = true;
    ::DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
  }
#endif

  _window = win;
  _renderer = makeOpenGLRenderer(this);
  if (!_renderer) { return false; }

  const auto [fw,fh] = _renderer->framebufferDimensions();
  _width = fw;
  _height = fh;

  //glfwSetInputMode(win, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
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

  glfwSetWindowUserPointer(win, this);
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
  if (_sizeSet) { showWindow(); }
  return true;
}

void WindowImpl::pollEvents()
{
  GX_ASSERT(isMainThread());
  glfwPollEvents();
    // callbacks will set event values
}

void WindowImpl::showWindow()
{
  glfwShowWindow(_window);

  // unmaximize if window started out maximized
  // (glfwShowWindow() does this if window is too large to fit on screen)
  glfwRestoreWindow(_window);

  // set initial mouse event state
  // (initial button state not supported by GLFW)
  updateMouseState();
}

void WindowImpl::updateMouseState()
{
  double mx = 0, my = 0;
  glfwGetCursorPos(_window, &mx, &my);
  const Vec2 mousePt{float(mx), float(my)};
  if (mousePt != _eventState.mousePt) {
    _eventState.events |= EVENT_MOUSE_MOVE;
    _eventState.mousePt = mousePt;
  }

  const bool mouseIn = (glfwGetWindowAttrib(_window, GLFW_HOVERED) != 0);
  if (mouseIn != _eventState.mouseIn) {
    _eventState.events |= EVENT_MOUSE_ENTER;
    _eventState.mouseIn = mouseIn;
  }
}

void WindowImpl::resetEventState()
{
  _eventState.events = 0;
  _eventState.scrollPt.set(0,0);
  _eventState.text.clear();

  // reset key states
  for (auto i = _eventState.keyStates.begin();
       i != _eventState.keyStates.end(); ) {
    if (i->held) {
      i->pressCount = 0;
      i->repeatCount = 0;
      ++i;
    } else {
      i = _eventState.keyStates.erase(i);
    }
  }

  // reset button states
  for (auto i = _eventState.buttonStates.begin();
       i != _eventState.buttonStates.end(); ) {
    if (i->held) {
      i->pressCount = 0;
      ++i;
    } else {
      i = _eventState.buttonStates.erase(i);
    }
  }

  // work-around for Windows where glfwSetWindowMonitor() isn't
  // triggering EVENT_SIZE
  if (_genSizeEvent) {
    _eventState.events |= EVENT_SIZE;
    updateMouseState();
    _genSizeEvent = false;
  }
}

bool WindowImpl::setupGLContext()
{
  setCurrentGLContext();
  return gx::setupGLContext(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress));
}

void WindowImpl::getGLFrameBufferSize(int& width, int& height)
{
  glfwGetFramebufferSize(_window, &width, &height);
}

void WindowImpl::setGLSwapInterval(int interval)
{
  glfwSwapInterval(interval);
}

void WindowImpl::setCurrentGLContext()
{
  // TODO: add lastThreadID check in debug
  // - if window hasn't changed, it must always be the same thread
  // - if window changes and threadID changes, call
  //   glfwMakeContextCurrent(nullptr) first to flush last context
  static GLFWwindow* lastWin = nullptr;
  if (lastWin != _window) {
    lastWin = _window;
    glfwMakeContextCurrent(_window);
  }
}

void WindowImpl::swapGLBuffers()
{
  glfwSwapBuffers(_window);
}

// GLFW event callbacks
void WindowImpl::closeCB(GLFWwindow* win)
{
  void* uPtr = glfwGetWindowUserPointer(win);
  if (!uPtr) {
    GX_LOG_ERROR("unknown close event");
    return;
  }

  //println("close event");
  static_cast<WindowImpl*>(uPtr)->_eventState.events |= EVENT_CLOSE;

  // tell GLFW not to close window
  glfwSetWindowShouldClose(win, GLFW_FALSE);
}

void WindowImpl::sizeCB(GLFWwindow* win, int width, int height)
{
  void* uPtr = glfwGetWindowUserPointer(win);
  if (!uPtr) {
    GX_LOG_ERROR("unknown size event");
    return;
  }

  //println("size event: ", width, ' ', height);
  auto impl = static_cast<WindowImpl*>(uPtr);
  impl->updateMouseState();
  if (width == impl->_width && height == impl->_height) { return; }

  impl->_eventState.events |= EVENT_SIZE;
  impl->_width = width;
  impl->_height = height;
  if (impl->_renderer) { impl->_renderer->setFramebufferSize(width, height); }
}

static constexpr KeyEnum translateGLFWKey(int key)
{
  switch (key) {
    default: return KEY_UNKNOWN;

    // printable keys
    case GLFW_KEY_SPACE:         return KEY_SPACE;
    case GLFW_KEY_APOSTROPHE:    return KEY_APOSTROPHE;
    case GLFW_KEY_COMMA:         return KEY_COMMA;
    case GLFW_KEY_MINUS:         return KEY_MINUS;
    case GLFW_KEY_PERIOD:        return KEY_PERIOD;
    case GLFW_KEY_SLASH:         return KEY_SLASH;
    case GLFW_KEY_0:             return KEY_0;
    case GLFW_KEY_1:             return KEY_1;
    case GLFW_KEY_2:             return KEY_2;
    case GLFW_KEY_3:             return KEY_3;
    case GLFW_KEY_4:             return KEY_4;
    case GLFW_KEY_5:             return KEY_5;
    case GLFW_KEY_6:             return KEY_6;
    case GLFW_KEY_7:             return KEY_7;
    case GLFW_KEY_8:             return KEY_8;
    case GLFW_KEY_9:             return KEY_9;
    case GLFW_KEY_SEMICOLON:     return KEY_SEMICOLON;
    case GLFW_KEY_EQUAL:         return KEY_EQUAL;
    case GLFW_KEY_A:             return KEY_A;
    case GLFW_KEY_B:             return KEY_B;
    case GLFW_KEY_C:             return KEY_C;
    case GLFW_KEY_D:             return KEY_D;
    case GLFW_KEY_E:             return KEY_E;
    case GLFW_KEY_F:             return KEY_F;
    case GLFW_KEY_G:             return KEY_G;
    case GLFW_KEY_H:             return KEY_H;
    case GLFW_KEY_I:             return KEY_I;
    case GLFW_KEY_J:             return KEY_J;
    case GLFW_KEY_K:             return KEY_K;
    case GLFW_KEY_L:             return KEY_L;
    case GLFW_KEY_M:             return KEY_M;
    case GLFW_KEY_N:             return KEY_N;
    case GLFW_KEY_O:             return KEY_O;
    case GLFW_KEY_P:             return KEY_P;
    case GLFW_KEY_Q:             return KEY_Q;
    case GLFW_KEY_R:             return KEY_R;
    case GLFW_KEY_S:             return KEY_S;
    case GLFW_KEY_T:             return KEY_T;
    case GLFW_KEY_U:             return KEY_U;
    case GLFW_KEY_V:             return KEY_V;
    case GLFW_KEY_W:             return KEY_W;
    case GLFW_KEY_X:             return KEY_X;
    case GLFW_KEY_Y:             return KEY_Y;
    case GLFW_KEY_Z:             return KEY_Z;
    case GLFW_KEY_LEFT_BRACKET:  return KEY_LBRACKET;
    case GLFW_KEY_BACKSLASH:     return KEY_BACKSLASH;
    case GLFW_KEY_RIGHT_BRACKET: return KEY_RBRACKET;
    case GLFW_KEY_GRAVE_ACCENT:  return KEY_BACKTICK;

    // function keys
    case GLFW_KEY_ESCAPE:        return KEY_ESCAPE;
    case GLFW_KEY_ENTER:         return KEY_ENTER;
    case GLFW_KEY_TAB:           return KEY_TAB;
    case GLFW_KEY_BACKSPACE:     return KEY_BACKSPACE;
    case GLFW_KEY_INSERT:        return KEY_INSERT;
    case GLFW_KEY_DELETE:        return KEY_DELETE;
    case GLFW_KEY_RIGHT:         return KEY_RIGHT;
    case GLFW_KEY_LEFT:          return KEY_LEFT;
    case GLFW_KEY_DOWN:          return KEY_DOWN;
    case GLFW_KEY_UP:            return KEY_UP;
    case GLFW_KEY_PAGE_UP:       return KEY_PAGE_UP;
    case GLFW_KEY_PAGE_DOWN:     return KEY_PAGE_DOWN;
    case GLFW_KEY_HOME:          return KEY_HOME;
    case GLFW_KEY_END:           return KEY_END;
    case GLFW_KEY_CAPS_LOCK:     return KEY_CAPS_LOCK;
    case GLFW_KEY_SCROLL_LOCK:   return KEY_SCROLL_LOCK;
    case GLFW_KEY_NUM_LOCK:      return KEY_NUM_LOCK;
    case GLFW_KEY_PRINT_SCREEN:  return KEY_PRINT_SCREEN;
    case GLFW_KEY_PAUSE:         return KEY_PAUSE;
    case GLFW_KEY_F1:            return KEY_F1;
    case GLFW_KEY_F2:            return KEY_F2;
    case GLFW_KEY_F3:            return KEY_F3;
    case GLFW_KEY_F4:            return KEY_F4;
    case GLFW_KEY_F5:            return KEY_F5;
    case GLFW_KEY_F6:            return KEY_F6;
    case GLFW_KEY_F7:            return KEY_F7;
    case GLFW_KEY_F8:            return KEY_F8;
    case GLFW_KEY_F9:            return KEY_F9;
    case GLFW_KEY_F10:           return KEY_F10;
    case GLFW_KEY_F11:           return KEY_F11;
    case GLFW_KEY_F12:           return KEY_F12;
    case GLFW_KEY_F13:           return KEY_F13;
    case GLFW_KEY_F14:           return KEY_F14;
    case GLFW_KEY_F15:           return KEY_F15;
    case GLFW_KEY_F16:           return KEY_F16;
    case GLFW_KEY_F17:           return KEY_F17;
    case GLFW_KEY_F18:           return KEY_F18;
    case GLFW_KEY_F19:           return KEY_F19;
    case GLFW_KEY_F20:           return KEY_F20;
    case GLFW_KEY_F21:           return KEY_F21;
    case GLFW_KEY_F22:           return KEY_F22;
    case GLFW_KEY_F23:           return KEY_F23;
    case GLFW_KEY_F24:           return KEY_F24;
    case GLFW_KEY_F25:           return KEY_F25;
    case GLFW_KEY_LEFT_SHIFT:    return KEY_LSHIFT;
    case GLFW_KEY_LEFT_CONTROL:  return KEY_LCONTROL;
    case GLFW_KEY_LEFT_ALT:      return KEY_LALT;
    case GLFW_KEY_LEFT_SUPER:    return KEY_LSUPER;
    case GLFW_KEY_RIGHT_SHIFT:   return KEY_RSHIFT;
    case GLFW_KEY_RIGHT_CONTROL: return KEY_RCONTROL;
    case GLFW_KEY_RIGHT_ALT:     return KEY_RALT;
    case GLFW_KEY_RIGHT_SUPER:   return KEY_RSUPER;
    case GLFW_KEY_MENU:          return KEY_MENU;

    // keypad
    case GLFW_KEY_KP_0:          return KEY_KP_0;
    case GLFW_KEY_KP_1:          return KEY_KP_1;
    case GLFW_KEY_KP_2:          return KEY_KP_2;
    case GLFW_KEY_KP_3:          return KEY_KP_3;
    case GLFW_KEY_KP_4:          return KEY_KP_4;
    case GLFW_KEY_KP_5:          return KEY_KP_5;
    case GLFW_KEY_KP_6:          return KEY_KP_6;
    case GLFW_KEY_KP_7:          return KEY_KP_7;
    case GLFW_KEY_KP_8:          return KEY_KP_8;
    case GLFW_KEY_KP_9:          return KEY_KP_9;
    case GLFW_KEY_KP_DECIMAL:    return KEY_KP_DECIMAL;
    case GLFW_KEY_KP_DIVIDE:     return KEY_KP_DIVIDE;
    case GLFW_KEY_KP_MULTIPLY:   return KEY_KP_MULTIPLY;
    case GLFW_KEY_KP_SUBTRACT:   return KEY_KP_SUBTRACT;
    case GLFW_KEY_KP_ADD:        return KEY_KP_ADD;
    case GLFW_KEY_KP_ENTER:      return KEY_KP_ENTER;
    case GLFW_KEY_KP_EQUAL:      return KEY_KP_EQUAL;
  }
}

void WindowImpl::keyCB(
  GLFWwindow* win, int key, int scancode, int action, int mods)
{
  void* uPtr = glfwGetWindowUserPointer(win);
  if (!uPtr) {
    GX_LOG_ERROR("unknown key event");
    return;
  }

  //println("key event: ", key, ' ', scancode, ' ', action, ' ', mods);
  EventState& es = static_cast<WindowImpl*>(uPtr)->_eventState;
  es.events |= EVENT_KEY;

  const KeyEnum val = translateGLFWKey(key);
  auto& states = es.keyStates;
  auto itr = std::find_if(states.begin(), states.end(),
			  [val](auto& ks){ return ks.key == val; });
  if (itr == states.end()) {
    itr = states.insert(states.end(), {val,int16_t(scancode),0,0,false});
  }

  KeyState& ks = *itr;
  if (action == GLFW_PRESS) {
    ++ks.pressCount;
    ks.held = true;
    switch (val) {
      default: break;
      case KEY_LSHIFT:   case KEY_RSHIFT:
        es.mods |= MODIFIER_SHIFT;   ++es.shiftCount; break;
      case KEY_LCONTROL: case KEY_RCONTROL:
        es.mods |= MODIFIER_CTRL; ++es.controlCount; break;
      case KEY_LALT:     case KEY_RALT:
        es.mods |= MODIFIER_ALT;     ++es.altCount; break;
      case KEY_LSUPER:   case KEY_RSUPER:
        es.mods |= MODIFIER_SUPER;   ++es.superCount; break;
    }
  } else if (action == GLFW_RELEASE) {
    ks.held = false;
    switch (val) {
      default: break;
      case KEY_LSHIFT:   case KEY_RSHIFT:
        if (--es.shiftCount <= 0) {
          es.shiftCount = 0; es.mods &= ~MODIFIER_SHIFT; }
        break;
      case KEY_LCONTROL: case KEY_RCONTROL:
        if (--es.controlCount <= 0) {
          es.controlCount = 0; es.mods &= ~MODIFIER_CTRL; }
        break;
      case KEY_LALT:     case KEY_RALT:
        if (--es.altCount <= 0) {
          es.altCount = 0; es.mods &= ~MODIFIER_ALT; }
        break;
      case KEY_LSUPER:   case KEY_RSUPER:
        if (--es.superCount <= 0) {
          es.superCount = 0; es.mods &= ~MODIFIER_SUPER; }
        break;
    }
  } else if (action == GLFW_REPEAT) {
    ++ks.repeatCount;
  }
}

void WindowImpl::charCB(GLFWwindow* win, unsigned int codepoint)
{
  void* uPtr = glfwGetWindowUserPointer(win);
  if (!uPtr) {
    GX_LOG_ERROR("unknown char event");
    return;
  }

  //println("char event: ", codepoint);
  EventState& es = static_cast<WindowImpl*>(uPtr)->_eventState;
  es.events |= EVENT_TEXT;
  es.text.append(toUTF8(codepoint));
}

void WindowImpl::cursorEnterCB(GLFWwindow* win, int entered)
{
  void* uPtr = glfwGetWindowUserPointer(win);
  if (!uPtr) {
    GX_LOG_ERROR("unknown cursor enter event");
    return;
  }

  //println("cursor enter event: ", entered);
  EventState& es = static_cast<WindowImpl*>(uPtr)->_eventState;
  es.events |= EVENT_MOUSE_ENTER;
  es.mouseIn = (entered != 0);
}

void WindowImpl::cursorPosCB(GLFWwindow* win, double xpos, double ypos)
{
  void* uPtr = glfwGetWindowUserPointer(win);
  if (!uPtr) {
    GX_LOG_ERROR("unknown cursor pos event");
    return;
  }

  //println("cursor pos event: ", xpos, ' ', ypos);
  EventState& es = static_cast<WindowImpl*>(uPtr)->_eventState;
  es.events |= EVENT_MOUSE_MOVE;
  es.mousePt.set(float(xpos), float(ypos));
}

void WindowImpl::mouseButtonCB(
  GLFWwindow* win, int button, int action, int mods)
{
  void* uPtr = glfwGetWindowUserPointer(win);
  if (!uPtr) {
    GX_LOG_ERROR("unknown mouse button even");
    return;
  }

  //println("mouse button event: ", button, ' ', action, ' ', mods);
  if (button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_8) {
    GX_LOG_ERROR("unknown mouse button ", button);
    return;
  }

  EventState& es = static_cast<WindowImpl*>(uPtr)->_eventState;;
  es.events |= EVENT_MOUSE_BUTTON;

  const int16_t bVal = int16_t(button) + 1;
  auto& states = es.buttonStates;
  auto itr = std::find_if(states.begin(), states.end(),
			  [bVal](auto& bs){ return bs.button == bVal; });
  if (itr == states.end()) {
    itr = states.insert(states.end(), {bVal,0,false});
  }

  ButtonState& bs = *itr;
  if (action == GLFW_PRESS) {
    ++bs.pressCount;
    bs.held = true;
  } else if (action == GLFW_RELEASE) {
    bs.held = false;
  }
}

void WindowImpl::scrollCB(GLFWwindow* win, double xoffset, double yoffset)
{
  void* uPtr = glfwGetWindowUserPointer(win);
  if (!uPtr) {
    GX_LOG_ERROR("unknown scroll event");
    return;
  }

  //println("scroll event: ", xoffset, ' ', yoffset);
  EventState& es = static_cast<WindowImpl*>(uPtr)->_eventState;
  es.events |= EVENT_MOUSE_SCROLL;
  es.scrollPt.x += float(xoffset);
  es.scrollPt.y += float(yoffset);
}

void WindowImpl::iconifyCB(GLFWwindow* win, int iconified)
{
  void* uPtr = glfwGetWindowUserPointer(win);
  if (!uPtr) {
    GX_LOG_ERROR("unknown iconify event");
    return;
  }

  //println("iconify event: ", iconified);
  EventState& es = static_cast<WindowImpl*>(uPtr)->_eventState;
  es.events |= EVENT_ICONIFY;
  es.iconified = iconified;
}

void WindowImpl::focusCB(GLFWwindow* win, int focused)
{
  void* uPtr = glfwGetWindowUserPointer(win);
  if (!uPtr) {
    GX_LOG_ERROR("unknown focus event");
    return;
  }

  //println("focus event: ", focused);
  EventState& es = static_cast<WindowImpl*>(uPtr)->_eventState;
  es.events |= EVENT_FOCUS;
  es.focused = focused;
}
