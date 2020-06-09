#include "os/Library.h"
#include <stdio.h>

int main(const int argc, const char *const argv[]) {
  int iterations = 300;
#ifdef __Tracer_h__
  iterations = 1;
#endif
  for (int i = 0; i < iterations; ++i) {
    typedef const char *(*zlibVersion)();
    for (int arg = 1; arg + 1 < argc; arg += 2) {
      try {
        printf("%s:%s->%p\n", argv[arg], argv[arg + 1],
               sys::Library(argv[arg]).function<void *>(argv[arg + 1]));
      } catch (const std::exception &exception) {
        fprintf(stderr, "EXCEPTION[arg=%d (\"%s\", \"%s\")]: %s\n", arg,
                argv[arg], argv[arg + 1], exception.what());
      }
    }
    try {
      printf("z:zlibVersion=%s\n",
             sys::Library("z").function<zlibVersion>("zlibVersion")());
    } catch (const std::exception &exception) {
      fprintf(stderr, "FAILED: EXCEPTION(z:zlibVersion): %s\n",
              exception.what());
    }
    try {
      printf("libz:zlibVersion=%s\n",
             sys::Library("libz").function<zlibVersion>("zlibVersion")());
    } catch (const std::exception &exception) {
      fprintf(stderr, "FAILED: EXCEPTION(libz:zlibVersion): %s\n",
              exception.what());
    }
    try {
      printf("z.dylib:zlibVersion=%s\n",
             sys::Library("z" __std_lib_suffix__)
                 .function<zlibVersion>("zlibVersion")());
    } catch (const std::exception &exception) {
      fprintf(stderr, "FAILED: EXCEPTION(z.dylib:zlibVersion): %s\n",
              exception.what());
    }
    try {
      printf("libz.dylib:zlibVersion=%s\n",
             sys::Library("libz" __std_lib_suffix__)
                 .function<zlibVersion>("zlibVersion")());
    } catch (const std::exception &exception) {
      fprintf(stderr, "FAILED: EXCEPTION(libz.dylib:zlibVersion): %s\n",
              exception.what());
    }
    try {
      printf("libz.dylib:zlibVersion=%s\n",
             sys::Library("/usr/lib/libz" __std_lib_suffix__)
                 .function<zlibVersion>("zlibVersion")());
    } catch (const std::exception &exception) {
      fprintf(stderr, "FAILED: EXCEPTION(libz.dylib:zlibVersion): %s\n",
              exception.what());
    }
#if defined(__APPLE__)
    typedef double (*absolute_time)();
    try {
      printf("CoreFoundation:CFAbsoluteTimeGetCurrent=%0.3f\n",
             sys::Library("CoreFoundation")
                 .function<absolute_time>("CFAbsoluteTimeGetCurrent")());
    } catch (const std::exception &exception) {
      fprintf(
          stderr,
          "FAILED: EXCEPTION(CoreFoundation:CFAbsoluteTimeGetCurrent): %s\n",
          exception.what());
    }
    try {
      printf("CoreFoundation.framework:CFAbsoluteTimeGetCurrent=%0.3f\n",
             sys::Library("CoreFoundation.framework")
                 .function<absolute_time>("CFAbsoluteTimeGetCurrent")());
    } catch (const std::exception &exception) {
      fprintf(
          stderr,
          "FAILED: "
          "EXCEPTION(CoreFoundation.framework:CFAbsoluteTimeGetCurrent): %s\n",
          exception.what());
    }
    try {
      printf("CoreFoundation.framework:CFAbsoluteTimeGetCurrent=%0.3f\n",
             sys::Library("/System/Library/Frameworks/CoreFoundation.framework")
                 .function<absolute_time>("CFAbsoluteTimeGetCurrent")());
    } catch (const std::exception &exception) {
      fprintf(
          stderr,
          "FAILED: "
          "EXCEPTION(CoreFoundation.framework:CFAbsoluteTimeGetCurrent): %s\n",
          exception.what());
    }
#endif
    try {
      sys::Library("Nonsense Library");
      fprintf(stderr, "FAILED: Expected an exception for Nonsense Library\n");
    } catch (const sys::Library::Exception &) {
    } catch (const std::exception &exception) {
      fprintf(stderr, "FAILED: Expected an exception for Nonsense Library\n");
    }
  }
  return 0;
}
