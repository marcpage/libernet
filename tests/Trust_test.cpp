#include "libernet/Trust.h"

#define dotest(condition)                                                      \
  if (!(condition)) {                                                          \
    fprintf(stderr, "FAIL(%s:%d) [%s]: %s\n", __FILE__, __LINE__, name, #condition);      \
  }

void validate(data::Trust &t, const char *name) {
	dotest(t.identities(keys) == 2);

	dotest(t.has(dummyIdentifier));
	dotest(t.has(yummyIdentifier));
	dotest(!t.has(summyIdentifier));

	dotest(t.trust(dummyIdentifier) == 1);
	dotest(t.trust(yummyIdentifier) == 2);
	dotest(t.trust(summyIdentifier) == 0);

	dotest(t.mistaken(dummyIdentifier) == 2);
	dotest(t.mistaken(yummyIdentifier) == 1);
	dotest(t.mistaken(summyIdentifier) == 0);

	dotest(t.disagree(dummyIdentifier) == 0);
	dotest(t.disagree(yummyIdentifier) == 1);
	dotest(t.disagree(summyIdentifier) == 0);

	dotest(t.malevolent(dummyIdentifier) == 1);
	dotest(t.malevolent(yummyIdentifier) == 0);
	dotest(t.malevolent(summyIdentifier) == 0);

	dotest(t.identities(keys) == 2);

	dotest(t.has(dummyIdentifier));
	dotest(t.has(yummyIdentifier));
	dotest(!t.has(summyIdentifier));
}

int main(const int /*argc*/, const char *const /*argv*/[]) {
  int iterations = 2;
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
		JSONData::List keys;

    	t1.addTrust(dummyIdentifier);
    	t1.addTrust(yummyIdentifier);
    	t1.addTrust(yummyIdentifier);

    	t1.addMistaken(dummyIdentifier);
    	t1.addMistaken(dummyIdentifier);
    	t1.addMistaken(yummyIdentifier);

    	t1.addDisagree(yummyIdentifier);

    	t1.addMalevolent(dummyIdentifier);

		validate(t1, "t1");

		t2 = t1;
		validate(t2, "t2");

    	data::Trust t3(t2);

		validate(t3, "t3");

		data::Trust t4(t3.data(), t3.identifier(), t3.key());

		validate(t4, "t4");

		data::Trust t5;

		t5.assign(t3.data(), t3.identifier(), t3.key());
		validate(t5, "t5");
    }
  } catch (const std::exception &e) {
    printf("FAIL: Exception: %s\n", e.what());
  }
  return 0;
}
