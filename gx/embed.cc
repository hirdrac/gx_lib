//
// gx/embed.cc
// Copyright (C) 2023 Richard Bradley
//
// command line tool to generate source with a file's contents as a static
// array (useful for embedding default font/image data into a binary so it's
// always available for use)
//

// TODO: add option to encode as 2/4/8 byte values
// TODO: option for assembly .incbin instead of C++ array
//       - .align, .globl, .incbin
//       - assembly include could be done with just a macro, but dependency
//         handling would need support in build

#include "CmdLineParser.hh"
#include "Print.hh"
#include <fstream>
#include <vector>
#include <cstdint>

// put in global namespace for convenience
using gx::print, gx::println, gx::println_err;


constexpr int ROW_SIZE = 16;


int showUsage(const char* const* argv)
{
  println("Usage: ", argv[0], " [options] <input file> <array name>");
  println("Options:");
  println("  -a,--alignas=[]  Alignment for data array");
  println("  -r,--rowsize=[]  Number of elements per row (default ", ROW_SIZE, ")");
  println("  -d,--dataonly    Only output array data (array name not required)");
  println("  --const          Declare all variables const");
  println("  --constexpr      Declare all variables constexpr");
  println("  -h,--help        Show usage");
  return 0;
}

int errorUsage(const char* const* argv)
{
  println_err("Try '", argv[0], " --help' for more information.");
  return -1;
}

int main(int argc, char** argv)
{
  std::string file, outVar;
  unsigned int alignVal = 0;
  int rowSize = ROW_SIZE;
  enum { NONE, CONST, CONSTEXPR } mode = NONE;
  bool dataOnly = false;

  for (gx::CmdLineParser p{argc, argv}; p; ++p) {
    if (p.option()) {
      if (p.option('h',"help")) {
        return showUsage(argv);
      } else if (p.option('a',"alignas", alignVal)) {
        if ((alignVal & (alignVal - 1)) != 0) {
          println_err("ERROR: alignas value not a power of 2");
          return errorUsage(argv);
        }
      } else if (p.option('r',"rowsize", rowSize)) {
        if (rowSize <= 0) { rowSize = ROW_SIZE; }
      } else if (p.option('d',"dataonly")) {
        dataOnly = true;
      } else if (p.option(0,"const")) {
        mode = CONST;
      } else if (p.option(0,"constexpr")) {
        mode = CONSTEXPR;
      } else {
        println_err("ERROR: bad option '", p.arg(), "'");
        return errorUsage(argv);
      }
    } else if (file.empty()) {
      p.get(file);
    } else if (outVar.empty()) {
      p.get(outVar);
    }
  }

  if (file.empty() || (outVar.empty() && !dataOnly)) {
    return errorUsage(argv);
  }

  std::ifstream fs{file, std::ios_base::binary};
  if (!fs) {
    println_err("Can't read file '", file, "'");
    return -1;
  }

  const std::size_t x = file.rfind('/');
  const std::string name = (x == std::string::npos) ? file : file.substr(x+1);
  std::vector<char> buffer;
  buffer.resize(std::size_t(rowSize));
  const char* prefix =
    (mode == CONST) ? "const " : ((mode == CONSTEXPR) ? "constexpr " : "");

  if (!dataOnly) {
    println("// generated from '", file, "'\n");
    println(prefix, "char ", outVar, "Name[] = \"", name, "\";");
    println(prefix, "char ", outVar, "File[] = \"", file, "\";");
    if (alignVal > 0) { print("alignas(", alignVal, ") "); }
    println(prefix, "unsigned char ", outVar, "[] = {");
  }

  for (;;) {
    fs.read(buffer.data(), std::streamsize(buffer.size()));
    auto len = std::streamsize(buffer.size());
    if (fs.eof()) {
      len = fs.gcount();
      if (len <= 0) { break; }
    }

    for (int i = 0; i < len; ++i) {
      const auto v = uint8_t(buffer[std::size_t(i)]);
      print(uint32_t{v}, ",");
    }
    println();
  }

  if (!dataOnly) {
    println("};");
    println(prefix, "unsigned long ", outVar, "Size = sizeof(", outVar, ");");
  }
  return 0;
}
