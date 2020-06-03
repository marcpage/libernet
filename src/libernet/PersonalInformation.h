#ifndef __PersonalInformation_h__
#define __PersonalInformation_h__

#include "libernet/Identity.h"
#include "libernet/OwnerIdentity.h"
#include "libernet/WrapperData.h"
#include "os/DateTime.h"
#include "os/Text.h"
#include "protocol/JSON.h"
#include <algorithm> // std::copy
#include <stdlib.h>  // rand
#include <string>
#include <vector>

namespace data {

class PersonalInformation : public WrapperData {
public:
  PersonalInformation() : WrapperData() {
    _changeInfo(json::Value().parse("{\"nickname\":\"\"}"));
  }
  PersonalInformation(const PersonalInformation &other) : WrapperData(other) {}
  PersonalInformation(const std::string &data, const std::string &identifier,
                      const std::string &key)
      : WrapperData(data, identifier, key) {}
  virtual ~PersonalInformation() {}
  PersonalInformation &operator=(const PersonalInformation &other);
  std::string nickname();
  void setNickname(const std::string &nickname);
  std::string next();
  void setNext(const std::string &next);
  bool valid();
  void invalidate();
  bool has(const std::string &key);
  std::string value(const std::string &key);
  void setValue(const std::string &key, const std::string &value);
  int credentialCount();
  void addCredential(const std::string &identifier, const std::string &key,
                     const std::string &filename);
  List &credentials(List &identifiers, ListAction action);
  std::string credentialIdentifier(int index);
  std::string credentialKey(int index);
  std::string credentialKey(const std::string &identifier) {
    return credentialKey(_index(identifier));
  }
  std::string credentialFilename(int index);
  std::string credentialFilename(const std::string &identifier) {
    return credentialFilename(_index(identifier));
  }
  int verifierCount();
  List &verifiers(List &identities, ListAction action);
  bool verify(Identity &signer);
  void validate(OwnerIdentity &owner);

private:
  int _index(const std::string &identifier);
  void _validate() override;
  std::string _wrapKey() override { return "identity"; }
};

inline PersonalInformation &
PersonalInformation::operator=(const PersonalInformation &other) {
  WrapperData::operator=(other);
  return *this;
}

inline std::string PersonalInformation::nickname() {
  return _info()["nickname"].string();
}

inline void PersonalInformation::setNickname(const std::string &nickname) {
  auto info = _info();
  info["nickname"] = nickname;
  _changeInfo(info);
}

inline std::string PersonalInformation::next() {
  return _info()["next"].string();
}

inline void PersonalInformation::setNext(const std::string &next) {
  auto info = _info();

  info["next"] = next;
  _changeInfo(info);
}

inline bool PersonalInformation::valid() {
  auto info = _info();

  return !info.has("valid") || info["value"].boolean();
}

/// @todo Test
inline void PersonalInformation::invalidate() {
  auto info = _info();

  AssertMessageException(info.has("next"));
  info["valid"] = false;
  _changeInfo(info);
}

inline bool PersonalInformation::has(const std::string &key) {
  return _info().has(key);
}

inline std::string PersonalInformation::value(const std::string &key) {
  return _info()[key].string();
}

inline void PersonalInformation::setValue(const std::string &key,
                                          const std::string &value) {
  auto info = _info();

  info[key] = value;
  _changeInfo(info);
}

inline int PersonalInformation::credentialCount() {
  return _info()["credentials"].count();
}

void PersonalInformation::addCredential(const std::string &identifier,
                                        const std::string &key,
                                        const std::string &filename) {
  auto info = _info();
  json::Value entry(json::ObjectType);

  entry["sha256"] = identifier;
  entry["aes256"] = key;
  entry["filename"] = filename;
  if (!info.has("credentials")) {
    info["credentials"].makeArray();
  }
  info["credentials"].append(entry);
  _changeInfo(info);
}

/// @todo Test
inline JSONData::List &
PersonalInformation::credentials(JSONData::List &identifiers,
                                 PersonalInformation::ListAction action) {
  auto info = _info();
  json::Value &credentials = info["credentials"];
  const int count = credentials.count();

  if (ClearFirst == action) {
    identifiers.clear();
  }

  for (int i = 0; i < count; ++i) {
    identifiers.push_back(credentials[i]["sha256"].string());
  }

  return identifiers;
}

inline std::string PersonalInformation::credentialIdentifier(int index) {
  return _info()["credentials"][index]["sha256"].string();
}

inline std::string PersonalInformation::credentialKey(int index) {
  return _info()["credentials"][index]["aes256"].string();
}

inline std::string PersonalInformation::credentialFilename(int index) {
  return _info()["credentials"][index]["filename"].string();
}

inline int PersonalInformation::verifierCount() {
  return JSONData::value()["verifiers"].count();
}

/// @todo Test
inline JSONData::List &
PersonalInformation::verifiers(JSONData::List &identities,
                               PersonalInformation::ListAction action) {
  auto keys = JSONData::value()["verifiers"].keys();

  if (ClearFirst == action) {
    identities.clear();
  }

  std::copy(keys.begin(), keys.end(), std::back_inserter(identities));
  return identities;
}

inline bool PersonalInformation::verify(Identity &signer) {
  auto wrapper = JSONData::value();
  json::Value &verifiers = wrapper["verifiers"];

  if (!verifiers.has(signer.identifier())) {
    return false; // not tested
  }

  return signer.valid(
      wrapper["identity"].string(),
      text::base64Decode(verifiers[signer.identifier()].string()));
}

inline void PersonalInformation::validate(OwnerIdentity &owner) {
  auto wrapper = JSONData::value();
  json::Value &verifiers = wrapper["verifiers"];
  verifiers[owner.identifier()] =
      text::base64Encode(owner.sign(wrapper["identity"].string()));
  _changeContent(wrapper);
}

inline int PersonalInformation::_index(const std::string &identifier) {
  auto info = _info();
  json::Value &credentials = info["credentials"];
  const int count = credentials.count();

  for (auto i = 0; i < count; ++i) {
    if (credentials[i]["sha256"].string() == identifier) {
      return i;
    }
  }
  ThrowMessageException("credential identifier not found " + // not tested
                        identifier);                         // not tested
}

inline void PersonalInformation::_validate() {
  auto wrapper = JSONData::value();
  auto info = _info(wrapper);

  WrapperData::_validate();
  JSONData::_validateKey(info, "nickname", json::StringType);

  JSONData::_validateOptionalKey(info, "valid", json::BooleanType);
  JSONData::_validateOptionalKey(info, "next", json::StringType);
  if (info.has("valid") && info["valid"].boolean()) {
    AssertMessageException(info.has("next")); // not tested
  }

  json::Value &credentials =
      JSONData::_validateOptionalKey(info, "credentials", json::ArrayType);

  if (credentials != info) {
    const auto count = credentials.count();

    for (auto i = 0; i < count; ++i) {
      json::Value &entry = credentials[i];

      JSONData::_validateKey(entry, "filename", json::StringType);
      JSONData::_validateHash(entry, "sha256");
      JSONData::_validateHash(entry, "aes256");
    }
  }

  JSONData::_validateOptionalKey(info, "image", json::StringType);
  JSONData::_validateOptionalKey(info, "first name", json::StringType);
  JSONData::_validateOptionalKey(info, "last name", json::StringType);
  JSONData::_validateOptionalKey(info, "domain", json::StringType);
  JSONData::_validateOptionalKey(info, "email", json::StringType);
  JSONData::_validateOptionalKey(info, "website", json::StringType);
  JSONData::_validateOptionalKey(info, "twitter", json::StringType);
  JSONData::_validateOptionalKey(info, "facebook", json::StringType);
  JSONData::_validateOptionalKey(info, "youtube", json::StringType);
  JSONData::_validateOptionalKey(info, "country", json::StringType);
  JSONData::_validateOptionalKey(info, "state", json::StringType);
  JSONData::_validateOptionalKey(info, "province", json::StringType);
  JSONData::_validateOptionalKey(info, "city", json::StringType);
  JSONData::_validateOptionalKey(info, "street", json::StringType);
  JSONData::_validateOptionalKey(info, "street number", json::StringType);
  JSONData::_validateOptionalKey(info, "apartment", json::StringType);

  json::Value &verifiers =
      JSONData::_validateOptionalKey(wrapper, "verifiers", json::ObjectType);
  const auto verifyingIdentifiers =
      (verifiers != wrapper) ? verifiers.keys() : json::Value::StringList();

  for (auto identifier : verifyingIdentifiers) {
    JSONData::_validateKey(verifiers, identifier, json::StringType);
  }
}

} // namespace data

#endif // __PersonalInformation_h__
