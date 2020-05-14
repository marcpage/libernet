#include "libernet/PersonalInformation.h"

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
    data::OwnerIdentity o1(512), o2(512), o3(512), o4(512);
    dt::DateTime start;
    std::string dummyIdentifier = hash::sha256("dummy").hex();
    data::Data dummyData("dummy", data::Data::Encrypted);
    data::Data yummyData("yummy", data::Data::Encrypted);

    for (int i = 0; i < iterations; ++i) {
      data::PersonalInformation p1, p2;

      p1.setNickname("Joe");
      p1.setNext(dummyIdentifier);
      p1.setValue("first name", "Joseph");
      p1.setValue("last name", "Brown");
      p1.setValue("image", dummyIdentifier);
      p1.setValue("domain", "apple.com");
      p1.setValue("twitter", "@marcpage");
      p1.setValue("facebook", "marcpage");
      p1.setValue("youtube", "marcallenpage");
      p1.setValue("country", "United States");
      p1.setValue("state", "Texas");
      p1.setValue("province", "Quebec");
      p1.setValue("city", "Austin");
      p1.setValue("postal code", "78701");
      p1.setValue("street", "Congress");
      p1.setValue("street number", "401");
      p1.setValue("unit", "1450");
      p1.addCredential(dummyData.identifier(), dummyData.key(), "dummy");
      p1.addCredential(yummyData.identifier(), yummyData.key(), "yummy");
      p1.setTimestamp(start);
      p1.sign(o1);
      p1.validate(o2);
      p1.validate(o3);
      p1.validate(o4);
      p1.calculate(2);

      dotest(p1.match() >= 2);
      dotest(p1.nickname() == "Joe");
      dotest(p1.next() == dummyIdentifier);
      dotest(p1.valid());
      dotest(p1.value("first name") == "Joseph");
      dotest(p1.value("last name") == "Brown");
      dotest(p1.value("image") == dummyIdentifier);
      dotest(p1.value("domain") == "apple.com");
      dotest(p1.value("twitter") == "@marcpage");
      dotest(p1.value("facebook") == "marcpage");
      dotest(p1.value("youtube") == "marcallenpage");
      dotest(p1.value("country") == "United States");
      dotest(p1.value("state") == "Texas");
      dotest(p1.value("province") == "Quebec");
      dotest(p1.value("city") == "Austin");
      dotest(p1.value("postal code") == "78701");
      dotest(p1.value("street") == "Congress");
      dotest(p1.value("street number") == "401");
      dotest(p1.value("unit") == "1450");
      dotest(fabs(p1.timestamp() - start) < 1.0);
      dotest(p1.credentialCount() == 2);

      dotest(p1.credentialIdentifier(0) == dummyData.identifier());
      dotest(p1.credentialKey(dummyData.identifier()) == dummyData.key());
      dotest(p1.credentialFilename(dummyData.identifier()) == "dummy");

      dotest(p1.credentialIdentifier(1) == yummyData.identifier());
      dotest(p1.credentialKey(yummyData.identifier()) == yummyData.key());
      dotest(p1.credentialFilename(yummyData.identifier()) == "yummy");

      dotest(p1.signer() == o1.identifier());
      dotest(p1.authenticate(o1));

      dotest(p1.verifierCount() == 3);
      dotest(p1.verify(o2));
      dotest(p1.verify(o3));
      dotest(p1.verify(o4));
    }
  } catch (const std::exception &e) {
    printf("FAIL: Exception: %s\n", e.what());
  }
  return 0;
}
