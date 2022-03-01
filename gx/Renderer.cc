//
// gx/Renderer.cc
// Copyright (C) 2022 Richard Bradley
//

#include "Renderer.hh"
#include "System.hh"
#include "Camera.hh"
#include <GLFW/glfw3.h>
#include <atomic>
#include <mutex>
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

bool gx::updateTexture(TextureID tid, const Image& img, int levels,
                       FilterType minFilter, FilterType magFilter)
{
  const std::lock_guard lg{_textureMutex};
  const auto itr = _textureOwners.find(tid);
  if (itr == _textureOwners.end()) { return false; }

  itr->second->setTexture(tid, img, levels, minFilter, magFilter);
  return true;
}

void gx::freeTexture(TextureID tid)
{
  const std::lock_guard lg{_textureMutex};
  const auto itr = _textureOwners.find(tid);
  if (itr != _textureOwners.end()) {
    itr->second->freeTexture(tid);
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

void Renderer::clearFrame(int width, int height)
{
  _width = width;
  _height = height;
  _layers.clear();
  _transforms.clear();
  _changed = true;
}

void Renderer::setScreenOrthoProjection(int layer)
{
  setTransform(
    layer, Mat4Identity, orthoProjection(float(_width), float(_height)));
}

void Renderer::setOrthoProjection(int layer, float width, float height)
{
  setTransform(layer, Mat4Identity, orthoProjection(width, height));
}

void Renderer::draw(int layer, const DrawEntry* data, std::size_t dataSize)
{
  if (dataSize == 0) { return; }

  DrawList& dl = _layers[layer].drawData;
  dl.insert(dl.end(), data, data + dataSize);
  _changed = true;
}

TextureID Renderer::newTextureID()
{
  const std::lock_guard lg{_textureMutex};
  const TextureID tid = ++_lastTextureID;
  _textureOwners.insert({tid,this});
  return tid;
}
