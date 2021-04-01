//
// gx/embed.cc
// Copyright (C) 2021 Richard Bradley
//
// command line tool to generate source with a file's contents as a static
// array (useful for embeding default font/image data into a binary so it's
// always available for use)

// TODO - add option to encode as 2/4/8 byte values

#include "Print.hh"
#include <fstream>

constexpr int ROW_SIZE = 16;


int main(int argc, char** argv)
{
  if (argc < 3) {
    gx::println_err("Usage: ", argv[0], " <input file> <array name>");
    return 0;
  }

  std::string file = argv[1];
  std::string outVar = argv[2];

  std::ifstream fs(file, std::ios_base::binary);
  if (!fs) {
    gx::println_err("Can't read file '", file, "'");
    return -1;
  }

  gx::println("// generated from '", file, "'\n");
  gx::print("unsigned char ", outVar, "[] = {");
  for (;;) {
    char val[ROW_SIZE];
    fs.read(val, ROW_SIZE);
    std::streamsize len = ROW_SIZE;
    if (fs.eof()) {
      len = fs.gcount();
      if (len == 0) { break; }
    }

    gx::println();
    for (int i = 0; i < len; ++i) {
      uint8_t v = uint8_t(val[i]);
      gx::print(uint32_t(v), ",");
    }
  }
  gx::println("\n};");
  gx::println("unsigned long ", outVar, "Size = sizeof(", outVar, ");");
  return 0;
}
