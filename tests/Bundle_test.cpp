#include "libernet/Bundle.h"

#define dotest(condition)                                                      \
  if (!(condition)) {                                                          \
    fprintf(stderr, "FAIL(%s:%d): %s\n", __FILE__, __LINE__, #condition);      \
  }

int main(const int /*argc*/, const char *const /*argv*/[]) {
  int iterations = 2;
#ifdef __Tracer_h__
  iterations = 1;
#endif
  try {
    const char *testFiles[] = {
        "tests/one_megabyte.jpg",   "tests/two_megabyte.jpg",
        "tests/Data_test.cpp",      "tests/JSONData_test.cpp",
        "tests/SmallFile_test.cpp",
    };
    io::Path sourceDir("bin/Bundle/Source");
    io::Path sourceSubDir("bin/Bundle/Source/Sub");
    io::Path destDir("bin/Bundle/Destination");
    sourceSubDir.mkdirs();
    for (size_t testIndex = 0;
         testIndex < sizeof(testFiles) / sizeof(testFiles[0]); ++testIndex) {
      io::Path src(testFiles[testIndex]);
      std::string filename = src.name();
      printf("Copying %s to %s and %s\n", std::string(src).c_str(),
             std::string(sourceDir + filename).c_str(),
             std::string(sourceSubDir + filename).c_str());
      src.copyContentsTo(sourceDir + filename)
          .copyContentsTo(sourceSubDir + filename);
    }
    for (int i = 0; i < iterations; ++i) {

      data::Bundle::Queue queue;

      data::Bundle source(sourceDir, queue);

      dotest(source == sourceDir);

      for (int j = 0; j < queue.size(); ++j) {
        data::Data temp = queue.dequeue();

        queue.enqueue(temp);
        if (i == 0) {
          printf("Block contents size = %ld\n", temp.contents().size());
        }
      }

      data::Bundle reconstituted(source.data(), source.identifier(),
                                 source.key());

      dotest(reconstituted != sourceDir);

      while (!queue.empty()) {
        data::Data restoreBlock = queue.dequeue();

        if (!reconstituted.write(destDir, restoreBlock)) {
          queue.enqueue(restoreBlock);
        }
      }

      dotest(source == destDir);
      dotest(reconstituted == destDir);
      dotest(reconstituted == sourceDir);
    }
  } catch (const std::exception &exception) {
    printf("FAIL: Exception thrown: %s\n", exception.what());
  }
  return 0;
}
