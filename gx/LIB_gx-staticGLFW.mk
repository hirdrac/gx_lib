# libgx.a build

LIB_gx = libgx
LIB_gx.SRC =\
  Logger.cc Unicode.cc Window.cc DrawList.cc Font.cc Image.cc Texture.cc\
  Gui.cc Renderer.cc OpenGLRenderer.cc OpenGL.cc glad.c stb_image.c
LIB_gx.PACKAGES = glfw3:3.2 freetype2
LIB_gx.TYPE = static
LIB_gx.STANDARD = c++17

# settings if using custom libglfw.a
LIB_gx.PACKAGES = freetype2
LIB_gx.LIBS = dl pthread GLFW/libglfw3.a
LIB_gx.INCLUDE = GLFW
