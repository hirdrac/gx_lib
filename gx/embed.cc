//
// gx/embed.cc
// Copyright (C) 2021 Richard Bradley
//
// command line tool to generate source with a file's contents as a static
// array (useful for embeding default font/image data into a binary so it's
// always available for use)
//

// TODO: add option to encode as 2/4/8 byte values
// TODO: 'const' & 'constexpr' options to add const/constexpr to definitions
// TODO: option for assemby .incbin instead of C++ array
//       - .align, .globl, .incbin
//       - assembly include could be done with just a macro, but dependency
//         handling would need support in build

#include "CmdLineParser.hh"
#include "Print.hh"
#include <fstream>


constexpr int ROW_SIZE = 16;


int showUsage(char** argv)
{
  gx::println("Usage: ", argv[0], " [options] <input file> <array name>");
  gx::println("Options:");
  gx::println("  -a,--alignas=[]  Alignment for data array");
  gx::println("  -h,--help        Show usage");
  return 0;
}

int errorUsage(char** argv)
{
  gx::println_err("Try '", argv[0], " --help' for more information.");
  return -1;
}

int main(int argc, char** argv)
{
  std::string file, outVar;
  unsigned int alignVal = 0;

  for (gx::CmdLineParser p(argc, argv); p; ++p) {
    if (p.option()) {
      if (p.option('h',"help")) {
        return showUsage(argv);
      } else if (p.option('a',"alignas", alignVal)) {
        // TODO: verify alignas is power of 2
      } else {
        gx::println_err("ERROR: bad option '", p.arg(), "'");
        return errorUsage(argv);
      }
    } else if (file.empty()) {
      p.get(file);
    } else if (outVar.empty()) {
      p.get(outVar);
    }
  }

  if (file.empty() || outVar.empty()) {
    return errorUsage(argv);
  }

  std::ifstream fs{file, std::ios_base::binary};
  if (!fs) {
    gx::println_err("Can't read file '", file, "'");
    return -1;
  }

  const std::size_t x = file.rfind('/');
  std::string name = (x == std::string::npos) ? file : file.substr(x+1);

  gx::println("// generated from '", file, "'\n");
  gx::println("char ", outVar, "Name[] = \"", name, "\";");
  gx::println("char ", outVar, "File[] = \"", file, "\";");
  if (alignVal > 0) { gx::print("alignas(", alignVal, ") "); }
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
      const auto v = uint8_t(val[i]);
      gx::print(uint32_t(v), ",");
    }
  }

  gx::println("\n};");
  gx::println("unsigned long ", outVar, "Size = sizeof(", outVar, ");");
  return 0;
}
