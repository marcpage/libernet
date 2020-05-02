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

      source.resetComment("This is a comment");

      data::Bundle copy(source);
      data::Bundle otherCopy;
      data::Bundle yetAnother;

      yetAnother.assign(source.data(), source.identifier(), source.key());

      otherCopy = source;
      dotest(otherCopy == source);
      dotest(source == copy);
      dotest(source != data::Bundle());
      dotest(source == sourceDir);
      dotest(otherCopy.comment() == "This is a comment");
      dotest(source.hasFile("one_megabyte.jpg"));
      dotest(source.hasFile("Sub/one_megabyte.jpg"));
      dotest(source.hasFile("two_megabyte.jpg"));
      dotest(source.hasFile("Sub/two_megabyte.jpg"));
      dotest(source.hasFile("JSONData_test.cpp"));
      dotest(source.hasFile("Sub/JSONData_test.cpp"));
      dotest(source.fileSize("JSONData_test.cpp") < 1024 * 1024);
      dotest(source.fileSize("one_megabyte.jpg") > 1024 * 1024);
      dotest(source.fileSize("one_megabyte.jpg") < 2 * 1024 * 1024);
      dotest(source.fileSize("two_megabyte.jpg") > 2 * 1024 * 1024);
      dotest(source.fileSize("two_megabyte.jpg") < 3 * 1024 * 1024);
      dotest(source.fileSize("Sub/JSONData_test.cpp") < 1024 * 1024);
      dotest(source.fileSize("Sub/one_megabyte.jpg") > 1024 * 1024);
      dotest(source.fileSize("Sub/one_megabyte.jpg") < 2 * 1024 * 1024);
      dotest(source.fileSize("Sub/two_megabyte.jpg") > 2 * 1024 * 1024);
      dotest(source.fileSize("Sub/two_megabyte.jpg") < 3 * 1024 * 1024);

      copy.resetComment();
      dotest(copy != source);
      dotest(copy.timestamp() != source.timestamp());

      otherCopy.addPreviousRevision(source.Data::identifier(),
                                    source.Data::key(), source.timestamp());

      dotest(otherCopy.hasPreviousRevision(source.Data::identifier()));
      dotest(source.previousRevisionCount() == 0);
      dotest(otherCopy.previousRevisionCount() == 1);
      dotest(otherCopy.previousRevisionIdentifier(0) ==
             source.Data::identifier());
      dotest(otherCopy.previousRevisionKey(0) == source.Data::key());
      dotest(otherCopy.previousRevisionTimestamp(0) == source.timestamp());

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
