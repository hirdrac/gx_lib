# embed build for Makefile.mk
# Copyright (C) 2026 Richard Bradley

BIN_embed.SRC = embed.cc
BIN_embed.STANDARD = c++20
BIN_embed.LIBS = -
BIN_embed.PACKAGES = -
BIN_embed.FLAGS = -
BIN_embed.RPATH = -
BIN_embed.LINK_FLAGS = -
BIN_embed.OPTIONS = modern_c++

ifeq ($(GX_LIB_PATH),)
  BIN_embed.SOURCE_DIR = gx
else
  BIN_embed.SOURCE_DIR = $(GX_LIB_PATH)/gx
endif
