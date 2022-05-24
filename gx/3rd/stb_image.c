// C file so implementation of stb_image is not compiled with C++ flags
// also location of implementation specific defines (STBI_*)

#ifdef GX_NDEBUG
#define STBI_ASSERT(expr) ((void)(0))
#endif

#define STBI_NO_FAILURE_STRINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
