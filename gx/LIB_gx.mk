# libgx.a build

LIB_gx = libgx
LIB_gx.SRC =\
  Logger.cc unicode.cc Window.cc DrawList.cc Font.cc Image.cc Texture.cc\
  Gui.cc Renderer.cc OpenGLRenderer.cc OpenGL.cc glad.c stb_image.c
LIB_gx.PACKAGES = glfw3:3.2 freetype2
LIB_gx.TYPE = static
LIB_gx.STANDARD = c++17
