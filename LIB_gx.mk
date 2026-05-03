#
# libgx build config
# Copyright (C) 2026 Richard Bradley
#
# external settings
# ~~~~~~~~~~~~~~~~~
# GX_LIB_PATH - set to library path (i.e. "../gx_lib")
# GX_FLAGS    - set if additional flags for library compile are needed
#               (don't set LIB_gx.FLAGS directly)
#

LIB_gx = libgx
LIB_gx.SRC =\
  Camera.cc Clipboard.cc DrawContext.cc Font.cc Gui.cc Image.cc\
  Logger.cc OpenGL.cc OpenGLRenderer.cc Renderer.cc TextFormat.cc\
  TextMetaState.cc ThreadID.cc Unicode.cc Window.cc\
  GLFW.cc WindowGLFWImpl.cc\
  3rd/glad_gl.c 3rd/stb_image.c

LIB_gx.LIBS = -
WINDOWS.LIB_gx.LIBS = Dwmapi

LIB_gx.PACKAGES = glfw3:3.2 freetype2
  # freetype2:24 for SDF support

ifeq ($(GX_LIB_PATH),)
  LIB_gx.SOURCE_DIR = gx
  ifeq ($(GX_FLAGS),)
    LIB_gx.FLAGS = -
  else
    LIB_gx.FLAGS = $(GX_FLAGS)
  endif
else
  LIB_gx.SOURCE_DIR = $(GX_LIB_PATH)/gx
  LIB_gx.FLAGS = $(GX_FLAGS) -fmacro-prefix-map=$(GX_LIB_PATH)/=
endif

LIB_gx.INCLUDE = $(LIB_gx.SOURCE_DIR)
