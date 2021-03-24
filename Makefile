# gx_lib makefile

include gx/LIB_gx.mk
LIB_gx.SOURCE_DIR = gx

# sample/demo programs
BIN1 = image_viewer
BIN1.SRC = image_viewer.cc
BIN1.LIBS = LIB_gx

BIN2 = text_viewer
BIN2.SRC = text_viewer.cc
BIN2.LIBS = LIB_gx

BIN3 = show_font
BIN3.SRC = show_font.cc
BIN3.LIBS = LIB_gx


BIN10 = demo_gui
BIN10.SRC = demo_gui.cc
BIN10.LIBS = LIB_gx

BIN11 = demo_draw
BIN11.SRC = demo_draw.cc
BIN11.LIBS = LIB_gx


STANDARD = c++17
OPTIONS = lto modern_c++
# NOTE: lto doesn't work for clang on MINGW64/MSys2
DEFINE = GX_DEBUG_GL
WARN_EXTRA = extra-semi fatal-errors conversion
#FLAGS_RELEASE = -DNDEBUG

include Makefile.mk

## END
