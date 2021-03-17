# gx_lib makefile

include gx/LIB_gx.mk
LIB_gx.SOURCE_DIR = gx

# sample/demo programs
BIN1 = image_viewer
BIN1.SRC = image_viewer.cc
BIN1.LIBS = LIB_gx

BIN2 = show_font
BIN2.SRC = show_font.cc
BIN2.LIBS = LIB_gx


BIN3 = demo_gui
BIN3.SRC = demo_gui.cc
BIN3.LIBS = LIB_gx

BIN4 = demo_draw
BIN4.SRC = demo_draw.cc
BIN4.LIBS = LIB_gx


STANDARD = c++17
OPTIONS = lto modern_c++
# NOTE: lto doesn't work for clang on MINGW64/MSys2
DEFINE = GX_DEBUG_GL
WARN_EXTRA = extra-semi fatal-errors conversion
#FLAGS_RELEASE = -DNDEBUG

include Makefile.mk

## END
