//
// gx/Renderer.cc
// Copyright (C) 2024 Richard Bradley
//

#include "Renderer.hh"
#include "System.hh"
#include <GLFW/glfw3.h>
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
void gx::TextureHandle::cleanup() noexcept
{
  const std::lock_guard lg{_textureMutex};
  const auto itr = _textureOwners.find(_id);
  if (itr != _textureOwners.end()) {
    itr->second->freeTexture(_id);
    _textureOwners.erase(itr);
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
  const std::lock_guard lg{_textureMutex};
  const TextureID tid = ++_lastTextureID;
  _textureOwners.insert({tid,this});
  return tid;
}
