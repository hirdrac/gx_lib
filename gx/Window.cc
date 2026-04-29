//
// gx/Window.cc
// Copyright (C) 2026 Richard Bradley
//

#include "Window.hh"
#include "WindowGLFWImpl.hh"
#include "EventState.hh"
#include "Time.hh"
#include "Assert.hh"
#include <vector>
#include <mutex>
using namespace gx;

namespace {
  // **** WindowImpl Instance Tracking ****
  std::vector<WindowImpl*> allImpls;
  std::mutex allImplsMutex;
}


// **** Window class ****
int64_t Window::_lastPollTime = 0;

Window::Window() : _impl{new WindowImpl}
{
  const std::lock_guard lg{allImplsMutex};
  allImpls.push_back(_impl.get());
}

Window::~Window()
{
  const std::lock_guard lg{allImplsMutex};
  std::erase(allImpls, _impl.get());
}

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
void Window::focus() {
  _impl->focus(); }
bool Window::isOpen() const {
  return _impl->_renderer != nullptr; }
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
MouseModeEnum Window::mouseMode() const {
  return _impl->_mouseMode; }
MouseShapeEnum Window::mouseShape() const {
  return _impl->_mouseShape; }
const gx::EventState& Window::eventState() const {
  return _impl->_eventState; }

int Window::pollEvents()
{
  const std::lock_guard lg{allImplsMutex};
  for (auto w : allImpls) {
    w->resetEventState();
  }

  WindowImpl::pollEvents();

  int e = 0;
  _lastPollTime = usecTime();
  for (auto w : allImpls) {
    EventState& es = w->_eventState;
    es.lastPollTime = _lastPollTime;
    e |= es.events;
  }

  return e;
}

Renderer& Window::renderer()
{
  GX_ASSERT(_impl->_renderer != nullptr);
  return *(_impl->_renderer);
}
