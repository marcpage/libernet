#ifndef __WrapperData_h__
#define __WrapperData_h__

#include "libernet/Identity.h"
#include "libernet/JSONData.h"
#include "libernet/OwnerIdentity.h"
#include "os/Text.h" // match
#include "protocol/JSON.h"
#include <stdlib.h> // rand
#include <string>
#include <vector>

namespace data {

class WrapperData : public JSONData {
public:
  WrapperData() : JSONData() {
    auto obj = json::Value(json::ObjectType);
    _changeInfo(obj);
  }
  WrapperData(const std::string &data, const std::string &identifier,
              const std::string &key)
      : JSONData(data, identifier, key) {}

  WrapperData(const WrapperData &other) : JSONData(other) {}
  virtual ~WrapperData() {}
  WrapperData &operator=(const WrapperData &other);
  void assign(const std::string &data, const std::string &identifier,
              const std::string &key);
  virtual void calculate(int matchCount);
  virtual int match();
  virtual dt::DateTime timestamp();
  virtual void setTimestamp(const dt::DateTime &timestamp = dt::DateTime()) {
    auto info = _info();

    _changeInfo(info, timestamp);
  }
  virtual std::string signer() { return JSONData::value()["signer"].string(); }
  virtual bool authenticate(Identity &signer);
  virtual void sign(OwnerIdentity &owner);

protected:
  virtual void _validate();
  virtual void _changeInfo(json::Value &info,
                           const dt::DateTime &timestamp = dt::DateTime(),
                           int matchCount = 0);
  virtual void _changeContent(json::Value &value, int matchCount = 0);
  virtual json::Value
  _info(const json::Value &wrapper = json::Value(json::NullType));
  virtual std::string _match() {
    if (Data::contents().size() == 0) {
      return std::string();
    }

    auto wrapper = JSONData::value();

    return wrapper.has("signer") ? wrapper["signer"].string() : std::string();
  }
  virtual std::string _wrapKey() { return std::string(); }
};

inline WrapperData &WrapperData::operator=(const WrapperData &other) {
  JSONData::operator=(other);
  return *this;
}

inline void WrapperData::assign(const std::string &data,
                                const std::string &identifier,
                                const std::string &key) {
  Data::assign(data, identifier, key);
  _validate();
}

inline void WrapperData::calculate(int matchCount) {
  auto wrapper = JSONData::value();
  _changeContent(wrapper, matchCount);
}

inline int WrapperData::match() {
  return text::matching(Data::identifier(), _match());
}

inline dt::DateTime WrapperData::timestamp() {
  return dt::DateTime(2001, dt::DateTime::Jan, 1, dt::DateTime::GMT) +
         double(_info()["timestamp"].integer());
}

inline bool WrapperData::authenticate(Identity &signer) {
  auto wrapper = JSONData::value();

  if (signer.identifier() != wrapper["signer"].string()) {
    return false; // not tested
  }
  return signer.valid(wrapper[_wrapKey()].string(),
                      text::base64Decode(wrapper["signature"].string()));
}

inline void WrapperData::sign(OwnerIdentity &owner) {
  auto wrapper = JSONData::value();

  wrapper["signer"] = owner.identifier();
  wrapper["signature"] =
      text::base64Encode(owner.sign(wrapper[_wrapKey()].string()));
  _changeContent(wrapper);
}

inline void WrapperData::_validate() {
  auto wrapper = JSONData::value();
  auto info = _info(wrapper);

  JSONData::_validatePositiveInteger(wrapper, "padding");
  JSONData::_validatePositiveInteger(info, "timestamp");

  JSONData::_validateBase64(wrapper, "signature", true);
  JSONData::_validateHash(wrapper, "signer", true);
}

inline void WrapperData::_changeInfo(json::Value &info,
                                     const dt::DateTime &timestamp,
                                     int matchCount) {
  json::Value wrapper(json::ObjectType);

  info["timestamp"] = int64_t(
      timestamp - dt::DateTime(2001, dt::DateTime::Jan, 1, dt::DateTime::GMT));
  wrapper[_wrapKey()] = info.format();
  _changeContent(wrapper, matchCount);
}

inline void WrapperData::_changeContent(json::Value &value, int matchCount) {
  const std::string matchIdentifier = _match();

  do {
    value["padding"] = int64_t(rand());
    JSONData::assign(value, Data::Unencrypted);
  } while (text::matching(Data::identifier(), matchIdentifier) < matchCount);
  _validate();
}

inline json::Value WrapperData::_info(const json::Value &wrapper) {
  return json::Value().parse(
      (wrapper.is(json::NullType) ? JSONData::value() : wrapper)[_wrapKey()]
          .string());
}

} // namespace data

#endif // __WrapperData_h__
