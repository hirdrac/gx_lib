# gx_lib makefile

include gx/LIB_gx.mk
LIB_gx.SOURCE_DIR = gx

# sample/demo programs
BIN1 = demo_gui
BIN1.SRC = demo_gui.cc
BIN1.LIBS = LIB_gx

BIN2 = image_viewer
BIN2.SRC = image_viewer.cc
BIN2.LIBS = LIB_gx


STANDARD = c++17
OPTIONS = lto modern_c++
DEFINE = DEBUG_GL
WARN_EXTRA = extra-semi
#FLAGS_RELEASE = -DNDEBUG

include Makefile.mk

## END
