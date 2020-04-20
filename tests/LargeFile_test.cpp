#include "libernet/LargeFile.h"

#define dotest(condition)                                                      \
  if (!(condition)) {                                                          \
    fprintf(stderr, "FAIL(%s:%d) on '%s': %s\n", __FILE__, __LINE__,           \
            std::string(path).c_str(), #condition);                            \
  }

int main(const int /*argc*/, const char *const /*argv*/[]) {
  int iterations = 5;
#ifdef __Tracer_h__
  iterations = 1;
#endif
  try {
    const char *testFiles[] = {
        "tests/one_megabyte.jpg", "tests/two_megabyte.jpg",
        "tests/Data_test.cpp",    "tests/JSONData_test.cpp",
        "tests/Package_test.cpp", "tests/SmallFile_test.cpp",
    };
    io::Path testDir("bin/LargeFile Restore");
    testDir.mkdirs();
    for (int i = 0; i < iterations; ++i) {
      for (size_t testIndex = 0;
           testIndex < sizeof(testFiles) / sizeof(testFiles[0]); ++testIndex) {
        io::Path path(testFiles[testIndex]);

        if (i == 0) {
          printf("Testing %lld byte file: %s\n", path.size(),
                 testFiles[testIndex]);
        }

        data::LargeFile::Queue queue;
        io::Path clone = testDir + path.name();
        data::Data block = data::loadFile(path, queue);
        data::LargeFile file;

        data::LargeFile::Queue dummy;

        if (i == 0) {
          printf("%s: queue size = %d file size = %lld\n",
                 std::string(path).c_str(), queue.size(), path.size());
        }

        dotest((queue.size() == 0) || (path.size() > 1024 * 1024));

        for (int j = 0; j < queue.size(); ++j) {
          data::Data temp = queue.dequeue();

          queue.enqueue(temp);
          if (i == 0) {
            printf("Block contents size = %ld\n", temp.contents().size());
          }
        }

        try {
          file = data::LargeFile(block.data(), block.identifier(), block.key());

          dotest(file == path);

          for (int j = 0; j < i + 1; ++j) {
            data::Data temp = queue.dequeue();
            queue.enqueue(temp);
          }

          while (!queue.empty()) {
            data::Data restoreBlock = queue.dequeue();

            dotest(file.write(clone, restoreBlock));
          }

          dotest(file == clone);
        } catch (const msg::Exception &exception) {
          if (i == 0) {
            fprintf(stderr, "EXCEPTION: %s\n", exception.what());
          }
          dotest(path.size() <= 1024 * 1024);
        }
      }
    }
  } catch (const std::exception &exception) {
    printf("FAIL: Exception thrown: %s\n", exception.what());
  }
  return 0;
}
