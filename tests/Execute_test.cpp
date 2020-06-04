#include "os/Execute.h"
#include <stdio.h>

int main(const int /*argc*/, const char *const /*argv*/[]) {
  std::string result, test;

  result = exec::execute("ls /", 4096);
  int iterations = 1700;
#ifdef __Tracer_h__
  iterations = 4096;
#endif
  for (size_t i = 1; i < iterations; ++i) {
    exec::execute("ls /", test, i);
    if (test != result) {
      printf("FAILED on blockSize=%ld\n", i);
      printf("=== EXPECTED ===\n%s\n", result.c_str());
      printf("=== RESULT ===\n%s\n", test.c_str());
    }
  }
  return 0;
}
