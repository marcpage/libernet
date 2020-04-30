#ifndef __Identity_h__
#define __Identity_h__

#include "libernet/Data.h"
#include "os/AsymmetricEncrypt.h"
#include "os/Path.h"
#include "protocol/JSON.h"

namespace data {

/// @todo Document
class Identity {
public:
  Identity() : _data(), _key() {}
  explicit Identity(const crypto::RSAAES256PublicKey &key)
      : _data(key.serialize(), data::Data::Unencrypted), _key(key) {}
  explicit Identity(const io::Path &file)
      : _data(_read(file)), _key(_convert(_data)) {}
  explicit Identity(const Data &data) : _data(data), _key(_convert(_data)) {}
  Identity(const Identity &other) : _data(other._data), _key(other._key) {}
  virtual ~Identity() {}
  Identity &operator=(const Identity &other) {
    _data = other._data;
    _key = other._key;
    return *this;
  }
  std::string identifier() { return _data.identifier(); }
  data::Data value() { return _data; }
  bool valid(const std::string &text, const std::string &signature) {
    return _key.verify(text, signature);
  }
  std::string &encrypt(const std::string &text) {
    std::string buffer;

    return _key.encrypt(text, buffer);
  }
  void write(const io::Path &file);

protected:
  static std::string toHex(const std::string &binary);
  static std::string fromHex(const std::string &hex);

private:
  data::Data _data;
  crypto::RSAAES256PublicKey _key;
  static crypto::RSAAES256PublicKey _convert(data::Data &data) {
    return crypto::RSAAES256PublicKey(data.contents(data::Data::Decompress));
  }
  static data::Data _read(const io::Path &file);
};

inline data::Data Identity::_read(const io::Path &file) {
  json::Value info(file.contents());

  return data::Data(fromHex(info["data"].string()),
                    info["identifier"].string());
}

inline void Identity::write(const io::Path &file) {
  json::Value info(json::ObjectType);
  std::string contents;

  info["identifier"] = _data.identifier();
  info["data"] = toHex(_data.data());
  info.format(contents);
  file.write(contents);
}

inline std::string Identity::toHex(const std::string &binary) {
  const char *const hexDigits = "0123456789abcdef";
  std::string value;

  for (int i = 0; (i < static_cast<int>(binary.size())); ++i) {
    const int lowerIndex = (binary[i] >> 4) & 0x0F;
    const int upperIndex = binary[i] & 0x0F;

    value.append(1, hexDigits[lowerIndex]);
    value.append(1, hexDigits[upperIndex]);
  }
  return value;
}

inline std::string Identity::fromHex(const std::string &hex) {
  std::string hexDigits("0123456789abcdef");
  std::string value;

  AssertMessageException(hex.size() % 2 == 0);
  for (int byte = 0; byte < static_cast<int>(hex.size() / 2); ++byte) {
    const int nibble1 = byte * 2 + 1;
    std::string::size_type found1 = hexDigits.find(hex[nibble1]);

    const int nibble2 = nibble1 - 1;
    std::string::size_type found2 = hexDigits.find(hex[nibble2]);

    AssertMessageException(found1 != std::string::npos);
    AssertMessageException(found2 != std::string::npos);
    value.append(1, (found2 << 4) | found1);
  }
  return value;
}

} // namespace data

#endif // __Identity_h__
