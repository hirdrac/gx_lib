//
// gx/Renderer.cc
// Copyright (C) 2021 Richard Bradley
//

#include "Renderer.hh"
#include "System.hh"
#include "Camera.hh"
#include <GLFW/glfw3.h>
#include <atomic>
#include <mutex>


// **** Texture Owner data/functions ****
namespace {
  std::unordered_map<gx::TextureID,gx::Renderer*> _textureOwners;
  std::mutex _textureOwnersMutex;

  void freeRenderer(gx::Renderer* rPtr)
  {
    std::lock_guard lg(_textureOwnersMutex);
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

void gx::registerTextureOwner(TextureID tid, Renderer* rPtr)
{
  if (tid > 0) {
    std::lock_guard lg(_textureOwnersMutex);
    _textureOwners[tid] = rPtr;
  }
}

bool gx::updateTexture(TextureID tid, const Image& img, int levels,
                       FilterType minFilter, FilterType magFilter)
{
  std::lock_guard lg(_textureOwnersMutex);
  auto itr = _textureOwners.find(tid);
  if (itr == _textureOwners.end()) { return false; }

  itr->second->setTexture(tid, img, levels, minFilter, magFilter);
  return true;
}

void gx::freeTexture(TextureID tid)
{
  std::lock_guard lg(_textureOwnersMutex);
  auto itr = _textureOwners.find(tid);
  if (itr != _textureOwners.end()) {
    itr->second->freeTexture(tid);
    _textureOwners.erase(itr);
  }
}


// **** Renderer class ****
gx::Renderer::~Renderer()
{
  if (_window && glfwInitStatus()) {
    glfwDestroyWindow(_window);
  }

  freeRenderer(this);
}

void gx::Renderer::clearFrame(int width, int height)
{
  _width = width;
  _height = height;
  _layers.clear();
  _transforms.clear();
  _changed = true;
}

void gx::Renderer::setScreenOrthoProjection(int layer)
{
  setTransform(
    layer, Mat4Identity, orthoProjection(float(_width), float(_height)));
}

void gx::Renderer::setOrthoProjection(int layer, float width, float height)
{
  setTransform(layer, Mat4Identity, orthoProjection(width, height));
}


// Static Methods
gx::TextureID gx::Renderer::newTextureID()
{
  static std::atomic<TextureID> _lastID = 0;
  return ++_lastID;
}
