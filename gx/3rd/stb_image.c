// C file so implementation of stb_image is not compiled with C++ flags
// also location of implementation specific defines (STBI_*)

#define STBI_NO_FAILURE_STRINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
