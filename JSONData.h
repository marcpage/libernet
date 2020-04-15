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

} // namespace data

#endif // __JSONData_h__
