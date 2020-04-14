#include "libernet/Data.h"

#define dotest(condition)                                                      \
  if (!(condition)) {                                                          \
    fprintf(stderr, "FAIL(%s:%d): %s\n", __FILE__, __LINE__, #condition);      \
  }

int main(const int /*argc*/, const char *const /*argv*/[]) {
  int iterations = 300;
#ifdef __Tracer_h__
  iterations = 1;
#endif
  try {
    const std::string unencryptedCompressed(
        "Unencrypted contents, but can be compressed "
        "ssssssssssssssssssssssssssss");
    const std::string encryptedCompressed(
        "Encrypted contents, but can be compressed "
        "ssssssssssssssssssssssssssss");
    const std::string tooBig(1024 * 1024 + 1, 0);
    const std::string unencryptedUncompressed("1");
    const std::string encryptedUncompressed("2");
    for (int i = 0; i < iterations; ++i) {
      data::Data d1, d2, d3, d4, d5, d6, d7, d8, d9, d10;

      dotest(data::Data().identifier() == hash::sha256(std::string("")).hex());
      dotest(data::Data().key() == "");
      dotest(data::Data().data() == "");
      dotest(data::Data().contents() == "");

      // with compression
      d1 = data::Data(unencryptedCompressed, data::Data::Unencrypted);
      d2 = data::Data(encryptedCompressed, data::Data::Encrypted);
      d3 = data::Data(d1.data(), d1.identifier());
      d4 = data::Data(d2.data(), d2.identifier(), d2.key());
      d5 = data::Data(d2.data(), d2.identifier());

      dotest(data::Data(unencryptedCompressed, data::Data::Unencrypted) ==
             data::Data(d1.data(), d1.identifier()));

      dotest(data::Data(d1.data(), d1.identifier()) ==
             data::Data(unencryptedCompressed, data::Data::Unencrypted));

      dotest(d1.contents().size() > d1.data().size());
      dotest(!d1.encrypted());
      dotest(d2.encrypted());
      dotest(!d3.encrypted());
      dotest(d4.encrypted());
      dotest(!d5.encrypted());

      dotest(d1 == d3);
      dotest(d2 == d4);
      dotest(d2 == d5);

      dotest(d1.contents(data::Data::Decompress) == unencryptedCompressed);
      dotest(d3.contents(data::Data::NoCompression) != unencryptedCompressed);
      dotest(d3.contents(data::Data::Decompress) == unencryptedCompressed);
      dotest(d4.contents() == encryptedCompressed);

      // no compression
      d6 = data::Data(unencryptedUncompressed, data::Data::Unencrypted);
      d7 = data::Data(encryptedUncompressed, data::Data::Encrypted);
      d8 = data::Data(d6.data(), d6.identifier());
      d9 = data::Data(d7.data(), d7.identifier(), d7.key());
      d10 = data::Data(d7.data(), d7.identifier());

      dotest(!d6.encrypted());
      dotest(d7.encrypted());
      dotest(!d8.encrypted());
      dotest(d9.encrypted());
      dotest(!d10.encrypted());

      dotest(d6 == d8);
      dotest(d7 == d9);
      dotest(d7 == d10);

      dotest(d6 != d1);
      dotest(d7 != d2);
      dotest(d8 != d3);
      dotest(d9 != d4);
      dotest(d10 != d5);

      dotest(d6.contents(data::Data::NoCompression) == unencryptedUncompressed);
      dotest(d8.contents(data::Data::NoCompression) == unencryptedUncompressed);
      dotest(d9.contents() == encryptedUncompressed);

      data::Data tmp = d1;

      d1 = d2;
      d2 = d3;
      d3 = d4;
      d4 = d5;
      d5 = d6;
      d6 = d7;
      d7 = d8;
      d8 = d9;
      d9 = d10;
      d10 = tmp;

      // with compression
      d1.assign(unencryptedCompressed, data::Data::Unencrypted);
      d2.assign(encryptedCompressed, data::Data::Encrypted);
      d3.assign(d1.data(), d1.identifier());
      d4.assign(d2.data(), d2.identifier(), d2.key());
      d5.assign(d2.data(), d2.identifier());

      dotest(!d1.encrypted());
      dotest(d2.encrypted());
      dotest(!d3.encrypted());
      dotest(d4.encrypted());
      dotest(!d5.encrypted());

      dotest(d1 == d3);
      dotest(d2 == d4);
      dotest(d2 == d5);

      dotest(d4.contents() == encryptedCompressed);

      // no compression
      d6.assign(unencryptedUncompressed, data::Data::Unencrypted);
      d7.assign(encryptedUncompressed, data::Data::Encrypted);
      d8.assign(d6.data(), d6.identifier());
      d9.assign(d7.data(), d7.identifier(), d7.key());
      d10.assign(d7.data(), d7.identifier());

      dotest(!d6.encrypted());
      dotest(d7.encrypted());
      dotest(!d8.encrypted());
      dotest(d9.encrypted());
      dotest(!d10.encrypted());

      dotest(d6 == d8);
      dotest(d7 == d9);
      dotest(d7 == d10);

      dotest(d6 != d1);
      dotest(d7 != d2);
      dotest(d8 != d3);
      dotest(d9 != d4);
      dotest(d10 != d5);

      dotest(d7.contents() == encryptedUncompressed);

      try {
        data::Data d(encryptedCompressed,
                     hash::sha256(encryptedCompressed).hex(),
                     hash::sha256(encryptedCompressed).hex());
        std::string s = d.contents();
        dotest(false /* We should have thrown */)
      } catch (const z::Exception &) {
      } catch (const data::CorruptData &) {
      }

      try {
        data::Data d(tooBig, data::Data::Unencrypted);
        dotest(false /* We should have thrown */)
      } catch (const data::DataTooBig &) {
      }

      try {
        data::Data d(tooBig, data::Data::Encrypted);
        dotest(false /* We should have thrown */)
      } catch (const data::DataTooBig &) {
      }

      try {
        data::Data d(tooBig, hash::sha256(tooBig).hex());
        dotest(false /* We should have thrown */)
      } catch (const data::DataTooBig &) {
      }

      try {
        data::Data d(unencryptedCompressed,
                     hash::sha256(encryptedCompressed).hex());
        dotest(false /* We should have thrown */)
      } catch (const data::InvalidIdentifier &) {
      }

      try {
        std::string compressed, data;
        hash::sha256 invalidKey = hash::sha256(unencryptedCompressed);

        z::compress(encryptedCompressed, compressed, 9);
        crypto::AES256 key(reinterpret_cast<const char *>(invalidKey.buffer()),
                           invalidKey.size());

        key.crypto::SymmetricKey::encryptInPlace(compressed, "", data);
        data::Data d(data, hash::sha256(data).hex(), invalidKey.hex());
        std::string value = d.contents();
        dotest(false /* We should have thrown */)
      } catch (const data::CorruptData &) {
      }
    }
  } catch (const std::exception &exception) {
    printf("FAIL: Exception thrown: %s\n", exception.what());
  }
  return 0;
}
