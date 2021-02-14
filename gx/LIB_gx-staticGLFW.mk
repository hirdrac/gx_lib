# libgx.a build

LIB_gx = libgx
LIB_gx.SRC =\
  Logger.cc Unicode.cc System.cc Window.cc DrawContext.cc Font.cc Image.cc\
  Texture.cc Camera.cc Gui.cc Renderer.cc OpenGLRenderer.cc OpenGL.cc\
  3rd/glad.c 3rd/stb_image.c
LIB_gx.PACKAGES = glfw3:3.2 freetype2
LIB_gx.TYPE = static
LIB_gx.STANDARD = c++17

# settings if using custom libglfw.a
LIB_gx.PACKAGES = freetype2
LIB_gx.LIBS = dl pthread GLFW/libglfw3.a
LIB_gx.INCLUDE = GLFW
