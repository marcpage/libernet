#include "libernet/SmallFile.h"

#define dotest(condition)                                                      \
  if (!(condition)) {                                                          \
    fprintf(stderr, "FAIL(%s:%d): %s\n", __FILE__, __LINE__, #condition);      \
  }

int main(const int /*argc*/, const char *const /*argv*/[]) {
  int iterations = 1100;
#ifdef __Tracer_h__
  iterations = 1;
#endif
  try {
    const char *testFiles[] = {
        "tests/Data_test.cpp",    "tests/JSONData_test.cpp",
        "tests/Package_test.cpp", "tests/SmallFile_test.cpp",
        "tests/one_megabyte.jpg", "tests/two_megabyte.jpg",
    };
    for (int i = 0; i < iterations; ++i) {
      for (size_t testIndex = 0;
           testIndex < sizeof(testFiles) / sizeof(testFiles[0]); ++testIndex) {
        io::Path file(testFiles[testIndex]);
        data::SmallFile fileData;

        if (i == 0) {
          printf("Testing %lld byte file: %s\n", file.size(),
                 testFiles[testIndex]);
        }
        if (file.isFile()) {
          try {
            fileData = data::SmallFile(file);
            dotest(file.size() <= 1024 * 1024);
            dotest(fileData == data::SmallFile(file));
            dotest(fileData ==
                   data::Data(fileData.data(), fileData.identifier()));
            dotest(fileData == data::Data(fileData.data(),
                                          fileData.identifier(),
                                          fileData.key()));
          } catch (const data::DataTooBig &) {
            dotest(file.size() > 1024 * 1024);
          }
        }
      }
    }
  } catch (const std::exception &exception) {
    printf("FAIL: Exception thrown: %s\n", exception.what());
  }
  return 0;
}
