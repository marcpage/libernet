#include "libernet/JSONData.h"

#define dotest(condition)                                                      \
  if (!(condition)) {                                                          \
    fprintf(stderr, "FAIL(%s:%d): %s\n", __FILE__, __LINE__, #condition);      \
  }

int main(const int /*argc*/, const char *const /*argv*/[]) {
  int iterations = 11000;
#ifdef __Tracer_h__
  iterations = 1;
#endif
  try {
    std::string compressableValue(
        "{\"test\":1,\"balloon\":[1,2,3,4],\"hello\":\"test\"}");
    std::string uncompressableValue("[1,2]");
    json::Value compressable(compressableValue);
    json::Value uncompressable(uncompressableValue);
    for (int i = 0; i < iterations; ++i) {
      data::JSONData d1, d2, d3, d4;

      d1 = data::JSONData(compressable, data::Data::Unencrypted);
      d2 = data::JSONData(uncompressable, data::Data::Unencrypted);
      d3 = data::JSONData(compressable, data::Data::Encrypted);
      d4 = data::JSONData(uncompressable, data::Data::Encrypted);
      dotest(d1.value().format() == d3.value().format());
      dotest(d2.value().format() == d4.value().format());
    }
  } catch (const std::exception &exception) {
    printf("FAIL: Exception thrown: %s\n", exception.what());
  }
  return 0;
}
