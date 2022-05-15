# libgx.a build w/ custom libglfw3.a

include LIB_gx.mk

LIB_gx.PACKAGES = freetype2
  # freetype2:24 for SDF support
LIB_gx.LIBS = dl pthread GLFW/libglfw3.a
LIB_gx.INCLUDE = GLFW
