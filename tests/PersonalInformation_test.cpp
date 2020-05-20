#include "libernet/PersonalInformation.h"

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
      dotest(p1.has("first name"));
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

      p2 = p1;

      dotest(p2.match() >= 2);
      dotest(p2.nickname() == "Joe");
      dotest(p2.next() == dummyIdentifier);
      dotest(p2.valid());
      dotest(p2.has("first name"));
      dotest(p2.value("first name") == "Joseph");
      dotest(p2.value("last name") == "Brown");
      dotest(p2.value("image") == dummyIdentifier);
      dotest(p2.value("domain") == "apple.com");
      dotest(p2.value("twitter") == "@marcpage");
      dotest(p2.value("facebook") == "marcpage");
      dotest(p2.value("youtube") == "marcallenpage");
      dotest(p2.value("country") == "United States");
      dotest(p2.value("state") == "Texas");
      dotest(p2.value("province") == "Quebec");
      dotest(p2.value("city") == "Austin");
      dotest(p2.value("postal code") == "78701");
      dotest(p2.value("street") == "Congress");
      dotest(p2.value("street number") == "401");
      dotest(p2.value("unit") == "1450");
      dotest(fabs(p2.timestamp() - start) < 1.0);
      dotest(p2.credentialCount() == 2);

      dotest(p2.credentialIdentifier(0) == dummyData.identifier());
      dotest(p2.credentialKey(dummyData.identifier()) == dummyData.key());
      dotest(p2.credentialFilename(dummyData.identifier()) == "dummy");

      dotest(p2.credentialIdentifier(1) == yummyData.identifier());
      dotest(p2.credentialKey(yummyData.identifier()) == yummyData.key());
      dotest(p2.credentialFilename(yummyData.identifier()) == "yummy");

      dotest(p2.signer() == o1.identifier());
      dotest(p2.authenticate(o1));

      dotest(p2.verifierCount() == 3);
      dotest(p2.verify(o2));
      dotest(p2.verify(o3));
      dotest(p2.verify(o4));

      dotest(data::PersonalInformation(p2).match() >= 2);
      dotest(data::PersonalInformation(p2).nickname() == "Joe");
      dotest(data::PersonalInformation(p2).next() == dummyIdentifier);
      dotest(data::PersonalInformation(p2).valid());
      dotest(data::PersonalInformation(p2).has("first name"));
      dotest(data::PersonalInformation(p2).value("first name") == "Joseph");
      dotest(data::PersonalInformation(p2).value("last name") == "Brown");
      dotest(data::PersonalInformation(p2).value("image") == dummyIdentifier);
      dotest(data::PersonalInformation(p2).value("domain") == "apple.com");
      dotest(data::PersonalInformation(p2).value("twitter") == "@marcpage");
      dotest(data::PersonalInformation(p2).value("facebook") == "marcpage");
      dotest(data::PersonalInformation(p2).value("youtube") == "marcallenpage");
      dotest(data::PersonalInformation(p2).value("country") == "United States");
      dotest(data::PersonalInformation(p2).value("state") == "Texas");
      dotest(data::PersonalInformation(p2).value("province") == "Quebec");
      dotest(data::PersonalInformation(p2).value("city") == "Austin");
      dotest(data::PersonalInformation(p2).value("postal code") == "78701");
      dotest(data::PersonalInformation(p2).value("street") == "Congress");
      dotest(data::PersonalInformation(p2).value("street number") == "401");
      dotest(data::PersonalInformation(p2).value("unit") == "1450");
      printf("timestamp diff = %0.3f start = %0.3f timestamp = %0.3f\n",
             (data::PersonalInformation(p2).timestamp() - start), double(start),
             double(data::PersonalInformation(p2).timestamp()));

      // FAILS, off by 6 hours, TZ
      // dotest(fabs(data::PersonalInformation(p2).timestamp() - start) < 1.0);
      dotest(data::PersonalInformation(p2).credentialCount() == 2);

      dotest(data::PersonalInformation(p2).credentialIdentifier(0) ==
             dummyData.identifier());
      dotest(data::PersonalInformation(p2).credentialKey(
                 dummyData.identifier()) == dummyData.key());
      dotest(data::PersonalInformation(p2).credentialFilename(
                 dummyData.identifier()) == "dummy");

      dotest(data::PersonalInformation(p2).credentialIdentifier(1) ==
             yummyData.identifier());
      dotest(data::PersonalInformation(p2).credentialKey(
                 yummyData.identifier()) == yummyData.key());
      dotest(data::PersonalInformation(p2).credentialFilename(
                 yummyData.identifier()) == "yummy");

      dotest(data::PersonalInformation(p2).signer() == o1.identifier());
      dotest(data::PersonalInformation(p2).authenticate(o1));

      dotest(data::PersonalInformation(p2).verifierCount() == 3);
      dotest(data::PersonalInformation(p2).verify(o2));
      dotest(data::PersonalInformation(p2).verify(o3));
      dotest(data::PersonalInformation(p2).verify(o4));

      data::PersonalInformation p3(p2.data(), p2.identifier(), p2.key());

      dotest(p3.match() >= 2);
      dotest(p3.nickname() == "Joe");
      dotest(p3.next() == dummyIdentifier);
      dotest(p3.valid());
      dotest(p3.has("first name"));
      dotest(p3.value("first name") == "Joseph");
      dotest(p3.value("last name") == "Brown");
      dotest(p3.value("image") == dummyIdentifier);
      dotest(p3.value("domain") == "apple.com");
      dotest(p3.value("twitter") == "@marcpage");
      dotest(p3.value("facebook") == "marcpage");
      dotest(p3.value("youtube") == "marcallenpage");
      dotest(p3.value("country") == "United States");
      dotest(p3.value("state") == "Texas");
      dotest(p3.value("province") == "Quebec");
      dotest(p3.value("city") == "Austin");
      dotest(p3.value("postal code") == "78701");
      dotest(p3.value("street") == "Congress");
      dotest(p3.value("street number") == "401");
      dotest(p3.value("unit") == "1450");
      dotest(fabs(p3.timestamp() - start) < 1.0);
      dotest(p3.credentialCount() == 2);

      dotest(p3.credentialIdentifier(0) == dummyData.identifier());
      dotest(p3.credentialKey(dummyData.identifier()) == dummyData.key());
      dotest(p3.credentialFilename(dummyData.identifier()) == "dummy");

      dotest(p3.credentialIdentifier(1) == yummyData.identifier());
      dotest(p3.credentialKey(yummyData.identifier()) == yummyData.key());
      dotest(p3.credentialFilename(yummyData.identifier()) == "yummy");

      dotest(p3.signer() == o1.identifier());
      dotest(p3.authenticate(o1));

      dotest(p3.verifierCount() == 3);
      dotest(p3.verify(o2));
      dotest(p3.verify(o3));
      dotest(p3.verify(o4));

      data::PersonalInformation p4;

      p4.assign(p3.data(), p3.identifier(), p3.key());

      dotest(p4.match() >= 2);
      dotest(p4.nickname() == "Joe");
      dotest(p4.next() == dummyIdentifier);
      dotest(p4.valid());
      dotest(p4.has("first name"));
      dotest(p4.value("first name") == "Joseph");
      dotest(p4.value("last name") == "Brown");
      dotest(p4.value("image") == dummyIdentifier);
      dotest(p4.value("domain") == "apple.com");
      dotest(p4.value("twitter") == "@marcpage");
      dotest(p4.value("facebook") == "marcpage");
      dotest(p4.value("youtube") == "marcallenpage");
      dotest(p4.value("country") == "United States");
      dotest(p4.value("state") == "Texas");
      dotest(p4.value("province") == "Quebec");
      dotest(p4.value("city") == "Austin");
      dotest(p4.value("postal code") == "78701");
      dotest(p4.value("street") == "Congress");
      dotest(p4.value("street number") == "401");
      dotest(p4.value("unit") == "1450");
      dotest(fabs(p4.timestamp() - start) < 1.0);
      dotest(p4.credentialCount() == 2);

      dotest(p4.credentialIdentifier(0) == dummyData.identifier());
      dotest(p4.credentialKey(dummyData.identifier()) == dummyData.key());
      dotest(p4.credentialFilename(dummyData.identifier()) == "dummy");

      dotest(p4.credentialIdentifier(1) == yummyData.identifier());
      dotest(p4.credentialKey(yummyData.identifier()) == yummyData.key());
      dotest(p4.credentialFilename(yummyData.identifier()) == "yummy");

      dotest(p4.signer() == o1.identifier());
      dotest(p4.authenticate(o1));

      dotest(p4.verifierCount() == 3);
      dotest(p4.verify(o2));
      dotest(p4.verify(o3));
      dotest(p4.verify(o4));
    }
  } catch (const std::exception &e) {
    printf("FAIL: Exception: %s\n", e.what());
  }
  return 0;
}
