#include "libernet/OwnerIdentity.h"
#include "os/Path.h"

#define dotest(condition)                                                      \
  if (!(condition)) {                                                          \
    fprintf(stderr, "FAIL(%s:%d): %s\n", __FILE__, __LINE__, #condition);      \
  }

int main(const int /*argc*/, const char *const /*argv*/[]) {
  int iterations = 15;
#ifdef __Tracer_h__
  iterations = 1;
#endif
  try {
    io::Path testDir("bin/OwnerIdentity test");

    testDir.mkdirs();
    for (int i = 0; i < iterations; ++i) {
      printf("Creating main owner\n");
      data::OwnerIdentity main(1024);
      printf("Creating copy owner\n");
      data::OwnerIdentity copy(main);
      printf("getting owner value\n");
      data::Data data = main.ownedValue("username", "password", 2); // 2=faster
      printf("Creating restored owner\n");
      data::OwnerIdentity restored(data, "password");
      io::Path mainStorage = testDir.uniqueName("private", ".data");
      io::Path publicStorage = testDir.uniqueName("public", ".data");
      data::OwnerIdentity empty;

      main.data::Identity::write(publicStorage);

      empty = main;
      printf("serializing\n");
      main.writeOwned(mainStorage, "password2");

      printf("creating last owner\n");
      data::OwnerIdentity loaded(mainStorage, "password2");
      data::Identity publicIdentity(publicStorage);

      dotest(publicIdentity.identifier() == main.data::Identity::identifier());

      data::Identity publicMain = main;
      std::string signedValue;
      std::string text = "";
      std::string encryptedValue;

      printf("testing empty string\n");
      signedValue = main.sign(text);
      dotest(signedValue == main.sign(text));
      dotest(signedValue == copy.sign(text));
      dotest(signedValue == restored.sign(text));
      dotest(signedValue == loaded.sign(text));
      dotest(signedValue == empty.sign(text));
      dotest(publicIdentity.valid(text, signedValue));
      encryptedValue = publicMain.encrypt(text);
      dotest(text == main.decrypt(encryptedValue));
      dotest(text == copy.decrypt(encryptedValue));
      dotest(text == restored.decrypt(encryptedValue));
      dotest(text == loaded.decrypt(encryptedValue));
      dotest(text == empty.decrypt(encryptedValue));

      printf("testing 'test'\n");
      text = "test";
      signedValue = main.sign(text);
      dotest(signedValue == main.sign(text));
      dotest(signedValue == copy.sign(text));
      dotest(signedValue == restored.sign(text));
      dotest(signedValue == loaded.sign(text));
      dotest(signedValue == empty.sign(text));
      dotest(publicIdentity.valid(text, signedValue));
      encryptedValue = publicMain.encrypt(text);
      dotest(text == main.decrypt(encryptedValue));
      dotest(text == copy.decrypt(encryptedValue));
      dotest(text == restored.decrypt(encryptedValue));
      dotest(text == loaded.decrypt(encryptedValue));
      dotest(text == empty.decrypt(encryptedValue));

      printf("testing large string\n");
      text = "Testing 01234567890123456789012345678901234567890123456789"
             "Testing 01234567890123456789012345678901234567890123456789"
             "Testing 01234567890123456789012345678901234567890123456789"
             "Testing 01234567890123456789012345678901234567890123456789"
             "Testing 01234567890123456789012345678901234567890123456789"
             "Testing 01234567890123456789012345678901234567890123456789";
      signedValue = main.sign(text);
      dotest(signedValue == main.sign(text));
      dotest(signedValue == copy.sign(text));
      dotest(signedValue == restored.sign(text));
      dotest(signedValue == loaded.sign(text));
      dotest(signedValue == empty.sign(text));
      dotest(publicIdentity.valid(text, signedValue));
      try {
        encryptedValue = publicMain.encrypt(text);
        dotest(false /* We can't encrypt something this big. */);
      } catch (const msg::Exception &) {
      }
    }
  } catch (const std::exception &exception) {
    printf("FAIL: Exception thrown: %s\n", exception.what());
  }
  return 0;
}
