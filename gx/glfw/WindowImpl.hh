//
// gx/glfw/WindowImpl.hh
// Copyright (C) 2026 Richard Bradley
//
// GLFW specific implementation
//

#include "Window.hh"
#include "EventState.hh"
#include <memory>
#include <string>

struct GLFWwindow;
namespace gx { class WindowImpl; }

class gx::WindowImpl
{
 public:
  WindowImpl();
  ~WindowImpl();

  void setTitle(std::string_view title);
  void setSize(int width, int height, bool fullScreen);
  void setSizeLimits(int minWidth, int minHeight, int maxWidth, int maxHeight);
  void setMouseMode(MouseMode mode);
  void setMouseShape(MouseShape shape);
  void setMousePos(Vec2 pos);
  void setSamples(int samples);
  bool open(int flags);
  void focus();

  static void pollEvents();
  void resetEventState();

  std::unique_ptr<Renderer> _renderer;

  // window states settings
  int _width = 0, _height = 0;
  std::string _title;
  MouseMode _mouseMode = MouseMode::normal;
  MouseShape _mouseShape = MouseShape::arrow;
  bool _fullScreen = false;

  // event state
  EventState _eventState{};

  // OpenGL handling
  bool setupGLContext();
  void getGLFrameBufferSize(int& width, int& height);
  void setGLSwapInterval(int interval);
  void setCurrentGLContext();
  void swapGLBuffers();

 private:
  GLFWwindow* _window = nullptr;
  int _fsWidth = 0, _fsHeight = 0;
  int _minWidth = -1, _minHeight = -1;
  int _maxWidth = -1, _maxHeight = -1;
  int _samples = 4; // for MSAA, 0 disables multi-sampling
  bool _sizeSet = false;
  bool _fixedAspectRatio = false;
  bool _genSizeEvent = false;

  void showWindow();
  void updateMouseState();

  static void closeCB(GLFWwindow* win);
  static void sizeCB(GLFWwindow* win, int width, int height);
  static void keyCB(
    GLFWwindow* win, int key, int scancode, int action, int mods);
  static void charCB(GLFWwindow* win, unsigned int codepoint);
  static void cursorEnterCB(GLFWwindow* win, int entered);
  static void cursorPosCB(GLFWwindow* win, double xpos, double ypos);
  static void mouseButtonCB(GLFWwindow* win, int button, int action, int mods);
  static void scrollCB(GLFWwindow* win, double xoffset, double yoffset);
  static void iconifyCB(GLFWwindow* win, int iconified);
  static void focusCB(GLFWwindow* win, int focused);
};
