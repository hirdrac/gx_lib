# gx_lib project makefile
# Copyright (C) 2021 Richard Bradley

include gx/LIB_gx.mk
LIB_gx.SOURCE_DIR = gx

include gx/BIN_embed.mk
BIN_embed.SOURCE_DIR = gx


# embeded font data generation
FILE1 = $(BUILD_TMP)/FixedWidthFontData.cc
FILE1.DEPS = BIN_embed data/LiberationMono-Regular.ttf
FILE1.CMD = ./$(DEP1) $(DEP2) FixedWidthFontData >$(OUT)

FILE2 = $(BUILD_TMP)/VariableWidthFontData.cc
FILE2.DEPS = BIN_embed data/FreeSans.ttf
FILE2.CMD = ./$(DEP1) $(DEP2) VariableWidthFontData >$(OUT)


# utility/sample programs
BIN1 = image_viewer
BIN1.SRC = image_viewer.cc
BIN1.LIBS = LIB_gx

BIN2 = text_viewer
BIN2.SRC = text_viewer.cc $(FILE1)
BIN2.LIBS = LIB_gx

BIN3 = show_font
BIN3.SRC = show_font.cc
BIN3.LIBS = LIB_gx


# feature demos/tests
BIN10 = demo_gui
BIN10.SRC = demo_gui.cc $(FILE2)
BIN10.LIBS = LIB_gx

BIN11 = demo_draw
BIN11.SRC = demo_draw.cc
BIN11.LIBS = LIB_gx


STANDARD = c++17
OPTIONS = lto modern_c++
# NOTE: lto doesn't work for clang on MINGW64/MSys2
DEFINE = GX_DEBUG_GL
WARN_EXTRA = extra-semi fatal-errors conversion cast-align cast-qual
#FLAGS_RELEASE = -DNDEBUG

include Makefile.mk

## END
