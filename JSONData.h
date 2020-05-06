#ifndef __JSONData_h__
#define __JSONData_h__

#include "libernet/Data.h"
#include "protocol/JSON.h"

namespace data {

class JSONData : public Data {
public:
  JSONData() : Data() {}
  explicit JSONData(const Data &data) : Data(data) {}
  virtual ~JSONData() {}
  JSONData(const json::Value &json, Data::Encryption encryption);
  JSONData(const std::string &data, const std::string &identifier);
  JSONData(const std::string &data, const std::string &identifier,
           const std::string &key);
  JSONData(const JSONData &other) : Data(other) {}
  JSONData &assign(const json::Value &json, Data::Encryption encryption);
  JSONData &operator=(const JSONData &other);
  json::Value value() {
    std::string buffer;
    return json::Value().parse(contents(buffer));
  }
  std::string &contents(std::string &buffer,
                        Compression compression = NoCompression) override;
  bool operator==(const Data &other) const { return Data::operator==(other); }
  bool operator!=(const Data &other) const { return !(*this == other); }

protected:
  static int64_t _validatePositiveInteger(json::Value &value,
                                          const std::string &name,
                                          bool optional = false);
  static void _validateHash(const json::Value &value);
  static void _validateHash(json::Value &value, const std::string &name,
                            bool optional = false);
  static json::Value &_validateKey(json::Value &value, const std::string &key,
                                   json::Type type, bool optional = false);
};

inline JSONData::JSONData(const json::Value &json, Data::Encryption encryption)
    : Data(json, encryption) {}

inline JSONData::JSONData(const std::string &data,
                          const std::string &identifier)
    : Data(data, identifier) {}

inline JSONData::JSONData(const std::string &data,
                          const std::string &identifier, const std::string &key)
    : Data(data, identifier, key) {}

inline JSONData &JSONData::assign(const json::Value &json,
                                  Data::Encryption encryption) {
  Data::assign(json, encryption);
  return *this;
}

inline JSONData &JSONData::operator=(const JSONData &other) {
  Data::operator=(other);
  return *this;
}

inline std::string &JSONData::contents(std::string &buffer,
                                       Compression compression) {
  return Data::contents(buffer, compression);
}

inline int64_t JSONData::_validatePositiveInteger(json::Value &value,
                                                  const std::string &name,
                                                  bool optional) {
  if (optional && !value.has(name)) {
    return -1; // not tested
  }
  AssertMessageException(value.has(name));
  AssertMessageException(value[name].is(json::IntegerType));
  AssertMessageException(value[name].integer() > 0);
  return value[name].integer();
}

inline void JSONData::_validateHash(const json::Value &value) {
  hash::sha256().reset(value.string().c_str());
}

inline void JSONData::_validateHash(json::Value &value, const std::string &name,
                                    bool optional) {
  if (optional && !value.has(name)) {
    return; // not tested
  }
  AssertMessageException(value.has(name));
  AssertMessageException(value[name].is(json::StringType));
  _validateHash(value[name]);
}

inline json::Value &JSONData::_validateKey(json::Value &value,
                                           const std::string &key,
                                           json::Type type, bool optional) {
  if (optional && !value.has(key)) {
    return value;
  }
  AssertMessageException(value.has(key));
  AssertMessageException(value[key].is(type));
  return value[key];
}

} // namespace data

#endif // __JSONData_h__
