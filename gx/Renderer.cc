//
// gx/Renderer.cc
// Copyright (C) 2026 Richard Bradley
//

#include "Renderer.hh"
#include "GLFW.hh"
#include <mutex>
#include <unordered_map>
using namespace gx;


// **** Texture Owner/RefCount data ****
namespace {
  struct TextureInfo {
    Renderer* owner;
    int refCount = 1;
  };

  std::mutex _textureMutex;
  std::unordered_map<TextureID,TextureInfo> _textures;
  TextureID _lastTextureID = 0;
}


// **** TextureHandle class ****
TextureHandle::TextureHandle(const TextureHandle& h) : _id{h._id}
{
  const std::lock_guard lg{_textureMutex};
  const auto itr = _textures.find(_id);
  if (itr != _textures.end()) { ++itr->second.refCount; }
}

TextureHandle& TextureHandle::operator=(const TextureHandle& h)
{
  if (_id != h._id) {
    cleanup();
    _id = h._id;

    const std::lock_guard lg{_textureMutex};
    const auto itr = _textures.find(_id);
    if (itr != _textures.end()) { ++itr->second.refCount; }
  }
  return *this;
}

void TextureHandle::cleanup() noexcept
{
  if (_id == 0) { return; }

  const std::lock_guard lg{_textureMutex};
  const auto itr = _textures.find(_id);
  if (itr == _textures.end()) { return; }

  TextureInfo& info = itr->second;
  if (--info.refCount > 0) { return; }

  info.owner->freeTexture(_id);
  _textures.erase(itr);
}


// **** Renderer class ****
Renderer::~Renderer()
{
  if (_window && glfwInitStatus()) {
    glfwDestroyWindow(_window);
  }

  // remove all textures for this Renderer
  const std::lock_guard lg{_textureMutex};
  auto itr = _textures.begin();
  while (itr != _textures.end()) {
    if (itr->second.owner == this) {
      _textures.erase(itr++);
    } else {
      ++itr;
    }
  }
}

TextureID Renderer::newTextureID()
{
  const std::lock_guard lg{_textureMutex};
  const TextureID tid = ++_lastTextureID;
  _textures.insert({tid, TextureInfo{ .owner = this }});
  return tid;
}
