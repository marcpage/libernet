#ifndef __OwnerIdentity_h__
#define __OwnerIdentity_h__

#include "libernet/Identity.h"
#include "os/AsymmetricEncrypt.h"
#include "protocol/JSON.h"
#include <stdlib.h>

namespace data {

class OwnerIdentity : public Identity {
public:
  OwnerIdentity() : Identity(), _key() {}
  OwnerIdentity(const int keySize = 4096)
      : Identity(), _key(keySize, _randomPublicExponent()) {
    Identity::operator=(_key.getPublicKey());
  }
  OwnerIdentity(const io::Path &file, const std::string &passphrase)
      : Identity(), _key() {
    data::Data block(file.contents(io::File::Binary), data::Data::Unencrypted);

    _decrypt(block, passphrase);
  }
  OwnerIdentity(Data &data, const std::string &passphrase)
      : Identity(), _key() {
    _decrypt(data, passphrase);
  }
  OwnerIdentity(const OwnerIdentity &other)
      : Identity(other), _key(other._key) {}
  virtual ~OwnerIdentity() {}
  OwnerIdentity &operator=(const OwnerIdentity &other) {
    Identity::operator=(other);
    _key = other._key;
    return *this;
  }
  data::Data ownedValue(const std::string &username,
                        const std::string &passphrase, int match = 6) {
    return _encrypt(username, passphrase, match);
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
    file.write(_encrypt("", passphrase, 0).contents(data::Data::Decompress),
               io::File::Binary);
  }

private:
  crypto::RSAAES256PrivateKey _key;
  static unsigned long _randomPublicExponent();
  data::Data _encrypt(const std::string &username,
                      const std::string &passphrase, int matchValue);
  void _decrypt(data::Data &data, const std::string &passphrase);
  static int _matching(const std::string &s1, const std::string &ss2);
  static std::string toHex(const std::string &binary);
  static std::string fromHex(const std::string &hex);
};

inline unsigned long OwnerIdentity::_randomPublicExponent() {
  static const unsigned long exponents[] = {3, 5, 17, 257, 65537};
  return exponents[rand() % (sizeof(exponents) / sizeof(exponents[0]))];
}

inline data::Data OwnerIdentity::_encrypt(const std::string &username,
                                          const std::string &passphrase,
                                          int matchValue) {
  json::Value info(json::ObjectType);
  std::string contents;
  std::string buffer;
  data::Data output;
  data::Data identity = value();
  hash::sha256 keyData(passphrase);
  crypto::AES256 key(reinterpret_cast<const char *>(keyData.buffer()),
                     keyData.size());
  std::string match(hash::sha256("private:" + username).hex());

  info["identifier"] = identity.identifier();
  info["public"] = toHex(identity.data());
  info["owner"] = _key.serialize(buffer);

  int best = 0;

  do {
    if (username.size() > 0) {
      info["padding"] = int64_t(rand());
    }
    info.format(contents); // crash
    key.crypto::SymmetricKey::encryptInPlace(contents, "", buffer);
    output = data::Data(buffer, data::Data::Unencrypted);
    if (_matching(match, output.identifier()) > best) {
      best = _matching(match, output.identifier());
      printf("match = %d\n", best);
    }
  } while ((username.size() == 0) ||
           (_matching(match, output.identifier()) < matchValue));

  return output;
}

inline void OwnerIdentity::_decrypt(data::Data &data,
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
  Identity::operator=(Identity(data::Data(fromHex(info["public"].string()),
                                          info["identifier"].string())));
  _key = crypto::RSAAES256PrivateKey(info["owner"].string());
}

inline int OwnerIdentity::_matching(const std::string &s1,
                                    const std::string &s2) {
  const int shorterLength = int(std::min(s1.size(), s2.size()));

  for (int i = 0; i < shorterLength; ++i) {
    if (s1[i] != s2[i]) {
      return i;
    }
  }
  return int(shorterLength);
}

inline std::string OwnerIdentity::toHex(const std::string &binary) {
  const char *const hexDigits = "0123456789abcdef";
  std::string value;

  for (int i = 0; (i < static_cast<int>(binary.size())); ++i) {
    value.append(1, hexDigits[binary[i] >> 4]);
    value.append(1, hexDigits[binary[i] & 0x0F]);
  }
  return value;
}

inline std::string OwnerIdentity::fromHex(const std::string &hex) {
  std::string hexDigits("0123456789abcdef");
  std::string value;
  printf("hex='%s'\n", hex.c_str());
  AssertMessageException(hex.size() % 2 == 0);
  for (int byte = 0; (byte < static_cast<int>(hex.size() / 2)); ++byte) {
    const int nibble1 = byte * 2 + 1;
    std::string::size_type found1 = hexDigits.find(hex[nibble1]);

    const int nibble2 = nibble1 - 1;
    std::string::size_type found2 = hexDigits.find(hex[nibble2]);
    printf("nibble1 = %c nibble2 = %c found1 = %ld found2=%ld\n", hex[nibble1],
           hex[nibble2], found1, found2);
    AssertMessageException(found1 != std::string::npos);
    AssertMessageException(found2 != std::string::npos);
    value.append(1, (found2 << 4) | found1);
  }
  return value;
}

} // namespace data

#endif // __OwnerIdentity_h__
