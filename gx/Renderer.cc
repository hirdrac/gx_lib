//
// gx/Renderer.cc
// Copyright (C) 2026 Richard Bradley
//

#include "Renderer.hh"
#include "GLFW.hh"
#include <mutex>
#include <unordered_map>
using namespace gx;


// **** Texture Owner data/functions ****
namespace {
  std::mutex _textureMutex;
  std::unordered_map<TextureID,Renderer*> _textureOwners;
  TextureID _lastTextureID = 0;


  void freeRenderer(Renderer* rPtr)
  {
    const std::lock_guard lg{_textureMutex};
    auto itr = _textureOwners.begin();
    auto end = _textureOwners.end();
    while (itr != end) {
      if (itr->second == rPtr) {
        _textureOwners.erase(itr++);
      } else {
        ++itr;
      }
    }
  }
}


// **** TextureHandle class ****
gx::TextureHandle::TextureHandle(const TextureHandle& h) : _id{h._id}
{
  const std::lock_guard lg{_textureMutex};
  const auto itr = _textureOwners.find(_id);
  if (itr != _textureOwners.end()) {
    itr->second->addTextureRef(_id);
  }
}

gx::TextureHandle& gx::TextureHandle::operator=(const TextureHandle& h)
{
  if (this != &h) {
    cleanup();
    _id = h._id;

    const std::lock_guard lg{_textureMutex};
    const auto itr = _textureOwners.find(_id);
    if (itr != _textureOwners.end()) {
      itr->second->addTextureRef(_id);
    }
  }
  return *this;
}

void gx::TextureHandle::cleanup() noexcept
{
  if (_id == 0) { return; }

  const std::lock_guard lg{_textureMutex};
  const auto itr = _textureOwners.find(_id);
  if (itr != _textureOwners.end()) {
    if (itr->second->removeTextureRef(_id) <= 0) {
      _textureOwners.erase(itr);
    }
  }
}


// **** Renderer class ****
Renderer::~Renderer()
{
  if (_window && glfwInitStatus()) {
    glfwDestroyWindow(_window);
  }

  freeRenderer(this);
}

TextureID Renderer::newTextureID()
{
  TextureID tid;
  {
    const std::lock_guard lg{_textureMutex};
    tid = ++_lastTextureID;
    _textureOwners.insert({tid,this});
  }
  return tid;
}
