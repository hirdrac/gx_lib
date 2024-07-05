# libgx library build

LIB_gx = libgx
LIB_gx.SRC =\
  Camera.cc DrawContext.cc Font.cc Gui.cc Image.cc Logger.cc\
  OpenGL.cc OpenGLRenderer.cc Renderer.cc System.cc TextFormat.cc\
  ThreadID.cc Unicode.cc Window.cc\
  3rd/glad_gl.c 3rd/stb_image.c

LIB_gx.LIBS = -
LIB_gx.PACKAGES = glfw3:3.2 freetype2
  # freetype2:24 for SDF support
