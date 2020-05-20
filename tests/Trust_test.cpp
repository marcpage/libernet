#include "libernet/Trust.h"

#define dotest(condition)                                                      \
  if (!(condition)) {                                                          \
    fprintf(stderr, "FAIL(%s:%d) [%s]: %s\n", __FILE__, __LINE__, name,        \
            #condition);                                                       \
  }

void validate(data::Trust &t, const char *name, const std::string &d,
              const std::string &y, const std::string &s) {
  data::JSONData::List keys;

  dotest(t.identities(keys).size() == 2);

  dotest(t.has(d));
  dotest(t.has(y));
  dotest(!t.has(s));

  dotest(t.trust(d) == 1);
  dotest(t.trust(y) == 2);
  dotest(t.trust(s) == 0);

  dotest(t.mistaken(d) == 2);
  dotest(t.mistaken(y) == 1);
  dotest(t.mistaken(s) == 0);

  dotest(t.disagree(d) == 0);
  dotest(t.disagree(y) == 1);
  dotest(t.disagree(s) == 0);

  dotest(t.malevolent(d) == 1);
  dotest(t.malevolent(y) == 0);
  dotest(t.malevolent(s) == 0);

  dotest(t.identities(keys).size() == 2);

  dotest(t.has(d));
  dotest(t.has(y));
  dotest(!t.has(s));
}

int main(const int /*argc*/, const char *const /*argv*/[]) {
  int iterations = 60;
#ifdef __Tracer_h__
  iterations = 1;
#endif
  try {
    data::OwnerIdentity o1(512), o2(512), o3(512), o4(512);
    dt::DateTime start;
    std::string dummyIdentifier = hash::sha256("dummy").hex();
    std::string yummyIdentifier = hash::sha256("yummy").hex();
    std::string summyIdentifier = hash::sha256("summy").hex();
    data::Data dummyData("dummy", data::Data::Encrypted);
    data::Data yummyData("yummy", data::Data::Encrypted);

    for (int i = 0; i < iterations; ++i) {
      data::Trust t1, t2;

      t1.addTrust(dummyIdentifier);
      t1.addTrust(yummyIdentifier);
      t1.addTrust(yummyIdentifier);

      t1.addMistaken(dummyIdentifier);
      t1.addMistaken(dummyIdentifier);
      t1.addMistaken(yummyIdentifier);

      t1.addDisagree(yummyIdentifier);

      t1.addMalevolent(dummyIdentifier);

      validate(t1, "t1", dummyIdentifier, yummyIdentifier, summyIdentifier);

      t2 = t1;
      validate(t2, "t2", dummyIdentifier, yummyIdentifier, summyIdentifier);

      data::Trust t3(t2);

      validate(t3, "t3", dummyIdentifier, yummyIdentifier, summyIdentifier);

      data::Trust t4(t3.data(), t3.identifier(), t3.key());

      validate(t4, "t4", dummyIdentifier, yummyIdentifier, summyIdentifier);

      data::Trust t5;

      t5.assign(t3.data(), t3.identifier(), t3.key());
      validate(t5, "t5", dummyIdentifier, yummyIdentifier, summyIdentifier);
    }
  } catch (const std::exception &e) {
    printf("FAIL: Exception: %s\n", e.what());
  }
  return 0;
}
