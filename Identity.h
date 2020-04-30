#ifndef __Identity_h__
#define __Identity_h__

#include "os/AsymmetricEncrypt.h"

namespace data {

class Identity {
public:
  Identity(const io::Path &file) : _data(_read(file)), _key(convert(_data)) {}
  Identity(const Data &data) : _data(data), _key(_convert(_data)) {}
  Identity(const Identity &other) : _data(other._data), _key(other._key) {}
  virtual ~Identity() {}
  Identity &operator=(const Identity &other) {
    _data = other._data;
    _key = other._key();
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

private:
  data::Data _data;
  crypto::RSAAES256PublicKey _key;
  static crypto::RSAAES256PublicKey _convert(data::Data &data) {
    return crypto::RSAAES256PublicKey(data.contents());
  }
  static data::Data _read(const io::Path &file);
};

inline data::Data Identity::_read(const io::Path &file) {
  json::Value info(file.read());

  return data::Data(info["data"].string(), info["identifier"].string());
}

inline void Identity::write(const io::Path &file) {
  json::Value info(json::ObjectType);
  std::string contents;

  info["identifier"] = _data.identifier();
  info["data"] = _data.data();
  info.format(contents);
  file.write(contents);
}

} // namespace data

#endif // __Identity_h__
