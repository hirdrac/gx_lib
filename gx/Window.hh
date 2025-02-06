//
// gx/Window.hh
// Copyright (C) 2025 Richard Bradley
//
// encapsulation of OS specific window handling
//

#pragma once
#include "Renderer.hh"
#include "EventState.hh"
#include "Types.hh"
#include <string_view>
#include <string>
#include <memory>


namespace gx {
  // Window Setting Values
  enum WindowFlagEnum {
    WINDOW_DECORATED = 1,
      // use decorations when not fullscreen

    // resizing flags
    WINDOW_RESIZABLE = 2,
      // can be resized when not fullscreen (decorated implied)
    WINDOW_FIXED_ASPECT_RATIO = 4,
      // set resize aspect ratio based on initial size
    WINDOW_LIMIT_MIN_SIZE = 8,
      // use initial size as min resize limit
    WINDOW_LIMIT_MAX_SIZE = 16,
      // use initial size as max resize limit

    // context flags
    WINDOW_DEBUG = 32,
      // enable OpenGL debug context
  };

  enum MouseModeEnum {
    MOUSEMODE_NORMAL,  // mouse cursor visible and behaves normally
    MOUSEMODE_HIDE,    // hides mouse cursor when it is over display window
    MOUSEMODE_DISABLE, // hides & grabs mouse cursor and all movement events
                       //   are relative position changes
  };

  enum MouseShapeEnum {
    MOUSESHAPE_ARROW = 0,
    MOUSESHAPE_IBEAM,
    MOUSESHAPE_CROSSHAIR,
    MOUSESHAPE_HAND,
    MOUSESHAPE_HRESIZE,
    MOUSESHAPE_VRESIZE,
  };


  class Window;
  class Renderer;
  struct WindowImpl;
}

class gx::Window
{
 public:
  Window();
  ~Window();

  // prevent copy/assignment
  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  // allow move/move-assign
  Window(Window&&) = default;
  Window& operator=(Window&&) = default;


  //// Display Management ////
  void setTitle(std::string_view title);
  void setSize(int width, int height, bool fullScreen);
  void setSizeLimits(int minWidth, int minHeight, int maxWidth, int maxHeight);
  void setMouseMode(MouseModeEnum mode);
  void setMouseShape(MouseShapeEnum shape);
  void setMousePos(Vec2 pos);
  void setSamples(int samples);

  bool open(int flags = WINDOW_RESIZABLE);
  [[nodiscard]] bool isOpen() const;
  [[nodiscard]] explicit operator bool() const { return isOpen(); }

  [[nodiscard]] int width() const;
  [[nodiscard]] int height() const;
  [[nodiscard]] std::pair<int,int> dimensions() const;
  [[nodiscard]] const std::string& title() const;
  [[nodiscard]] bool fullScreen() const;
  [[nodiscard]] MouseModeEnum mouseMode() const;
  [[nodiscard]] MouseShapeEnum mouseShape() const;


  //// Event Handling ////
  static int pollEvents();
    // updates event state for all windows, returns combined event mask
    // (each window should be checked for events if returned value is non-zero)

  [[nodiscard]] static int64_t lastPollTime() { return _lastPollTime; }
    // time of last pollEvents()
    // (in microseconds since first window open)

  [[nodiscard]] const EventState& eventState() const;
    // current event state for window

  void removeEvent(int event_mask);
    // remove event(s) from current event mask

  // renderer access methods
  [[nodiscard]] Renderer& renderer();
    // NOTE: Renderer is available once open() is called and will be available
    //   until the Window is destroyed.

  template<class... Args>
  void draw(Args&&... args) { renderer().draw({&args...}); }

  void renderFrame() { renderer().renderFrame(_lastPollTime); }

 private:
  std::unique_ptr<WindowImpl> _impl;
  static int64_t _lastPollTime;
};
