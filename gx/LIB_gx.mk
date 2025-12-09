#
# libgx build config
# Copyright (C) 2025 Richard Bradley
#

LIB_gx = libgx
LIB_gx.SRC =\
  Camera.cc Clipboard.cc DrawContext.cc Font.cc GLFW.cc Gui.cc Image.cc\
  Logger.cc OpenGL.cc OpenGLRenderer.cc Renderer.cc TextFormat.cc\
  TextMetaState.cc ThreadID.cc Unicode.cc Window.cc\
  3rd/glad_gl.c 3rd/stb_image.c

LIB_gx.LIBS = -
WINDOWS.LIB_gx.LIBS = Dwmapi

LIB_gx.PACKAGES = glfw3:3.2 freetype2
  # freetype2:24 for SDF support
