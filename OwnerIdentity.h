#ifndef __OwnedIdentity_h__
#define __OwnedIdentity_h__

#include "libernet/Identity.h"
#include "os/AsymmetricEncrypt.h"
#include "protocol/JSON.h"
#include <stdlib.h>

namespace data {

class OwnedIdentity : public Identity {
public:
  OwnedIdentity() : _key() {}
  OwnedIdentity(const int keySize = 4096)
      : Identity(), _key(keySize, _randomPublicExponent()) {}
  OwnedIdentity(const io::Path &file, const std::string &passphrase) : _key() {
    _decrypt(
        data::Data(file.contents(io::File::Binary), data::Data::Unencrypted));
  }
  OwnedIdentity(const Data &data, const std::string &passphrase)
      : Identity(), _key() {
    _decrypt(data, passphrase);
  }
  OwnedIdentity(const OwnedIdentity &other)
      : Identity(other), _key(other._key) {}
  virtual ~OwnedIdentity() {}
  OwnedIdentity &operator=(const OwnedIdentity &other) {
    Identiter::operator=(other);
    _key = other._key();
    return *this;
  }
  data::Data ownedValue(const std::string &passphrase) {
    return _encrypt(passphrase);
  }
  std::string sign(const std::string &text) {
    std::string buffer;

    return _key.sign(text, buffer);
  }
  std::string decrypt(const std::string &encrypted) {
    std::string buffer;

    return _key.decrypt(encrypted, buffer);
  }
  void writeOwned(const io::Path &file, const std::string &passphrase) {
    file.write(_encrypt(passphrase).contents(data::Data::Decompress),
               io::File::Binary);
  }

private:
  crypt::RSAAES256PrivateKey _key;
  static unsigned long _randomPublicExponent();
  data::Data _encrypt(const std::string &username,
                      const std::string &passphrase);
  void _decrypt(data::Data &data, const std::string &passphrase);
  static int _matching(const std::string &s1, const std::string &ss2);
};

inline unsigned long OwnedIdentity::_randomPublicExponent() {
  static const unsigned long exponents[] = {3, 5, 17, 257, 65537};
  return exponents[rand() % (sizeof(exponents) / sizeof(exponents[0]))];
}

inline data::Data OwnedIdentity::_encrypt(const std::string &username,
                                          const std::string &passphrase) {
  json::Value info(json::ObjectType);
  std::string contents;
  std::string buffer;
  data::Data output;
  data::Data identity = value();
  hash::sha256 keyData(passphrase);
  crypto::AES256 key(reinterpret_cast<const char *>(keyData.buffer()),
                     keyData.size());
  std::string match("private:" + username).hex();

  info["identifier"] = identity.identifier();
  info["public"] = identifier.data();
  info["owner"] = _key.serialize(buffer);

  do {
    info["padding"] = int64_t(rand());
    info.format(contents);
    key.crypt::SymmetricKey::encryptInPlace(contents, "", buffer);
    output = data::Data(buffer, data::Data::Unencrypted);
  } while (_matching(match, output.identifier()) < 6);

  return output;
}

inline void OwnedIdentity::_decrypt(data::Data &data,
                                    const std::string &passphrase) {
  json::Value info;
  std::string contents;
  data::Data output;
  hash::sha256 keyData(passphrase);
  crypto::AES256 key(reinterpret_cast<const char *>(keyData.buffer()),
                     keyData.size());

  key.crypto::SymmetricKey::decryptInPlace(
      data.contents(data::Data::Decompress), "", contents);
  info.parse(contents);
  Identity::operator=(Identity(data::Data(info["public"], info["identifier"])));
  _key = RSAAES256PrivateKey(info["owner"]);
}

inline int OwnedIdentity::_matching(const std::string &s1,
                                    const std::string &s2) {
  const auto shorterLength = std::min(s1.size(), s2.size());

  for (decltype(shorterLength) i = 0; i < shorterLength; ++i) {
    if (s1[i] != s2[i]) {
      return int(i);
    }
  }
  return int(shorterLength);
}

} // namespace data

#endif // __OwnedIdentity_h__
