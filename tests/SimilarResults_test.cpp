#include "libernet/SimilarResults.h"

#define dotest(condition)                                                      \
  if (!(condition)) {                                                          \
    fprintf(stderr, "FAIL(%s:%d): %s\n", __FILE__, __LINE__, #condition);      \
  }

int main(const int /*argc*/, const char *const /*argv*/[]) {
  int iterations = 10;
#ifdef __Tracer_h__
  iterations = 1;
#endif
  try {
    std::string identifier1 = hash::sha256("test1").hex();
    std::string identifier2 = hash::sha256("test2").hex();
    std::string identifier3 = hash::sha256("test3").hex();
    std::string identifier4 = hash::sha256("test4").hex();
    std::string identifier5 = hash::sha256("test5").hex();
    std::string identifier6 = hash::sha256("test6").hex();

    for (int i = 0; i < iterations; ++i) {
      data::SimilarResults r1, r2, r3;
      data::JSONData::List list;

      dotest(r1 == r2);

      r1.add(identifier1, identifier2, 5);
      r1.add(identifier1, identifier3, 6);
      r1.add(identifier1, identifier4, 7);
      r1.add(identifier3, identifier4, 10);
      r1.add(identifier5, identifier6, 15);

      dotest(r1 != r2);
      dotest(r1.size(identifier1, identifier2) == 5);
      dotest(r1.size(identifier1, identifier3) == 6);
      dotest(r1.size(identifier1, identifier4) == 7);
      dotest(r1.size(identifier3, identifier4) == 10);
      dotest(r1.size(identifier5, identifier6) == 15);
      dotest(r1.size(identifier1, identifier5) == -1);
      dotest(r1.size(identifier5, identifier1) == -1);
      dotest(r1.size(identifier2, identifier6) == -1);
      dotest(r1.size(identifier4, identifier6) == -1);
      dotest(r1.searches(list).size() == 3);
      dotest(r1.results(identifier1, list).size() == 3);
      dotest(r1.results(identifier3, list).size() == 1);
      dotest(r1.results(identifier5, list).size() == 1);
      dotest(r1.results(identifier2, list).size() == 0);

      r2 = r1;

      dotest(r1 == r2);
      r1.remove(identifier3, identifier4);
      r1.remove(identifier1);
      dotest(r1 != r2);
      dotest(r2.size(identifier1, identifier2) == 5);
      dotest(r2.size(identifier1, identifier3) == 6);
      dotest(r2.size(identifier1, identifier4) == 7);
      dotest(r2.size(identifier3, identifier4) == 10);
      dotest(r2.size(identifier5, identifier6) == 15);
      dotest(r2.size(identifier1, identifier5) == -1);
      dotest(r2.size(identifier5, identifier1) == -1);
      dotest(r2.size(identifier2, identifier6) == -1);
      dotest(r2.size(identifier4, identifier6) == -1);
      dotest(r2.searches(list).size() == 3);
      dotest(r2.results(identifier1, list).size() == 3);
      dotest(r2.results(identifier3, list).size() == 1);
      dotest(r2.results(identifier5, list).size() == 1);
      dotest(r2.results(identifier2, list).size() == 0);

      r3.assign(r2.data(), r2.identifier());

      dotest(r1 != r3);
      dotest(r2 == r3);
      dotest(r3.size(identifier1, identifier2) == 5);
      dotest(r3.size(identifier1, identifier3) == 6);
      dotest(r3.size(identifier1, identifier4) == 7);
      dotest(r3.size(identifier3, identifier4) == 10);
      dotest(r3.size(identifier5, identifier6) == 15);
      dotest(r3.size(identifier1, identifier5) == -1);
      dotest(r3.size(identifier5, identifier1) == -1);
      dotest(r3.size(identifier2, identifier6) == -1);
      dotest(r3.size(identifier4, identifier6) == -1);
      dotest(r3.searches(list).size() == 3);
      dotest(r3.results(identifier1, list).size() == 3);
      dotest(r3.results(identifier3, list).size() == 1);
      dotest(r3.results(identifier5, list).size() == 1);
      dotest(r3.results(identifier2, list).size() == 0);

      data::SimilarResults r4(r3);

      dotest(r1 != r4);
      dotest(r2 == r4);
      dotest(r3 == r4);
      dotest(r4.size(identifier1, identifier2) == 5);
      dotest(r4.size(identifier1, identifier3) == 6);
      dotest(r4.size(identifier1, identifier4) == 7);
      dotest(r4.size(identifier3, identifier4) == 10);
      dotest(r4.size(identifier5, identifier6) == 15);
      dotest(r4.size(identifier1, identifier5) == -1);
      dotest(r4.size(identifier5, identifier1) == -1);
      dotest(r4.size(identifier2, identifier6) == -1);
      dotest(r4.size(identifier4, identifier6) == -1);
      dotest(r4.searches(list).size() == 3);
      dotest(r4.results(identifier1, list).size() == 3);
      dotest(r4.results(identifier3, list).size() == 1);
      dotest(r4.results(identifier5, list).size() == 1);
      dotest(r4.results(identifier2, list).size() == 0);

      data::SimilarResults r5(r4.data(), r4.identifier());

      dotest(r1 != r5);
      dotest(r2 == r5);
      dotest(r3 == r5);
      dotest(r4 == r5);
      dotest(r5.size(identifier1, identifier2) == 5);
      dotest(r5.size(identifier1, identifier3) == 6);
      dotest(r5.size(identifier1, identifier4) == 7);
      dotest(r5.size(identifier3, identifier4) == 10);
      dotest(r5.size(identifier5, identifier6) == 15);
      dotest(r5.size(identifier1, identifier5) == -1);
      dotest(r5.size(identifier5, identifier1) == -1);
      dotest(r5.size(identifier2, identifier6) == -1);
      dotest(r5.size(identifier4, identifier6) == -1);
      dotest(r5.searches(list).size() == 3);
      dotest(r5.results(identifier1, list).size() == 3);
      dotest(r5.results(identifier3, list).size() == 1);
      dotest(r5.results(identifier5, list).size() == 1);
      dotest(r5.results(identifier2, list).size() == 0);
    }
  } catch (const std::exception &e) {
    printf("FAIL: Exception: %s\n", e.what());
  }
  return 0;
}
