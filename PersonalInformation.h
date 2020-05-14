#ifndef __PersonalInformation_h__
#define __PersonalInformation_h__

#include "libernet/JSONData.h"
#include "os/Text.h"
#include "protocol/JSON.h"
#include <algorithm> // std::copy
#include <string>
#include <vector>
#include <stdlib.h>  // rand

namespace data {

class PersonalInformation : public JSONData {
public:
  enum ListAction { ClearFirst, Append };
  typedef std::vector<std::string> List;
  PersonalInformation() : JSONData() {
    _changeInfo(json::Value().parse("\"nickname\":\"\""));
  }
  PersonalInformation(const PersonalInformation &other) : JSONData(other) {}
  PersonalInformation(const std::string &data, const std::string &identifier,
                      const std::string &key)
      : JSONData(data, identifier, key) {}
  virtual ~PersonalInformation() {}
  PersonalInformation &operator=(const PersonalInformation &other);
  PersonalInformation &assign(const std::string &data,
                              const std::string &identifier,
                              const std::string &key);
  PersonalInformation &calculate(int matchCount);
  int match();
  std::string nickname();
  void setNickname(const std::string &nickname);
  dt::DateTime timestamp();
  void setTimestamp(const dt::DateTime &timestamp);
  std::string next();
  void setNext(const std::string &next);
  bool valid();
  void invalidate();
  bool has(const std::string &key);
  std::string value(const std::string &key);
  void setValue(const std::string &key, const std::string &value);
  int credentialCount();
  void addCredential(const std::string &identifier, const std::string &key, const std::string &filename);
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
  std::string signer();
  bool authenticate(Identity &signer);
  void sign(OwnerIdentity &owner);
  int verifierCount();
  List &verifiers(List &identities, ListAction action);
  bool verify(Identity &signer);
  void validate(OwnerIdentity &owner);

private:
  int _index(const std::string &identifier);
  void _validate();
  void _changeInfo(json::Value &info,
                   const dt::DateTime &timestamp = dt::DateTime(),
                   int matchCount = 0);
  void _changeContent(json::Value &value, int matchCount = 0);
  json::Value _info(const json::Value &wrapper=json::Value(json::NullType));
};

inline PersonalInformation::PersonalInformation(const std::string &data,
                                                const std::string &identifier,
                                                const std::string &key)
    : JSONData(data, identifier, key) {}

inline PersonalInformation &
PersonalInformation::operator=(const PersonalInformation &other) {
  JSONData::operator=(other);
  return *this;
}

inline PersonalInformation &
PersonalInformation::assign(const std::string &data,
                            const std::string &identifier,
                            const std::string &key) {
  Data::assign(data, identifier, key);
  _validate();
  return *this;
}

inline PersonalInformation &PersonalInformation::calculate(int matchCount) {
  auto wrapper = JSONData::value();
  _changeContent(wrapper, matchCount);
  return *this;
}

inline int PersonalInformation::match() {
  return text::matching(Data::identifier(), signer());
}

inline std::string PersonalInformation::nickname() {
  return _info()["nickname"].string();
}

inline void PersonalInformation::setNickname(const std::string &nickname) {
  auto info = _info();
  info["nickname"] = nickname;
  _changeInfo(info);
}

inline dt::DateTime PersonalInformation::timestamp() {
  return dt::DateTime(2001, dt::DateTime::Jan, 1, dt::DateTime::GMT) +
         double(_info()["timestamp"].integer());
}

inline void PersonalInformation::setTimestamp(const dt::DateTime &timestamp) {
  auto info = _info();

  _changeInfo(info, timestamp);
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

  return info.has("valid") && info["value"].boolean();
}

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
  return _info()[key];
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

void PersonalInformation::addCredential(const std::string &identifier, const std::string &key, const std::string &filename) {
  auto info = _info();
	json::Value entry(json::ObjectType);

	entry["sha256"] = identifier;
	entry["aes256"] = key;
	entry["filename"] = filename;
	if (!info.has("credentials")) {
		info["credentials"].make(json::ArrayType);
	}
	info["credentials"].append(entry);
  _changeInfo(info);
}

inline PersonalInformation::List &PersonalInformation::credentials(PersonalInformation::List &identifiers,
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

inline std::string PersonalInformation::signer() {
  return JSONData::value()["signer"].string();
}

inline bool PersonalInformation::authenticate(Identity &signer) {
  auto wrapper = JSONData::value();

	if (signer.identifier() != wrapper["signer"].string()) {
		return false;
	}
	return signer.valid(wrapper["identity"].string(), text::base64Decode(wrapper["signature"].string()));
}

inline void PersonalInformation::sign(OwnerIdentity &owner) {
  auto wrapper = JSONData::value();

  wrapper["signer"] = signer.identifier();
  wrapper["signature"] = text::base64Encode(owner.sign(wrapper["identity"].string()));
  _changeContent(wrapper);
}

inline int PersonalInformation::verifierCount() {
  return JSONData::value()["verifiers"].count();
}

inline PersonalInformation::List &PersonalInformation::verifiers(PersonalInformation::List &identities,
                                            PersonalInformation::ListAction action) {
  auto wrapper = JSONData::value();
  json::Value &verifiers = wrapper["verifiers"];
  auto keys = verifiers.keys();

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
  	return false;
  }

  return signer.valid(wrapper["identity"].string(), text::base64Decode(verifiers[signer.identifier()].string()));
}

inline void PersonalInformation::validate(OwnerIdentity &owner) {
  auto wrapper = JSONData::value();
  json::Value &verifiers = wrapper["verifiers"];
	verifiers[owner.identifier()] = text::base64Encode(owner.sign(wrapper["identity"].string()));
  _changeContent(wrapper);
}

inline int PersonalInformation::_index(const std::string &identifier) {
  auto info = _info();
  json::Value &credentials = info["credentails"];
  const int count = credentials.count();

  for (i = 0; i < count; ++i) {
    if (credentials[i]["sha256"].string() == identifier) {
      return i;
    }
  }
  ThrowMessageException("credential identifier not found " +
                        identifier); // not tested
}

inline void PersonalInformation::_validate() {
  auto wrapper = JSONData::value();
  auto info = _info(wrapper);


	JSONData::_validatePositiveInteger(wrapper, "padding");
	JSONData::_validatePositiveInteger(wrapper, "timestamp");
	JSONData::_validateKey(info, "nickname", json::StringType);

	JSONData::_validateKey(info, "valid", json::BooleanType, true);
	JSONData::_validateKey(info, "next", json::StringType, true);
	if (info.has("valid") && info["valid"].boolean()) {
		AssertMessageException(info.has("next"));
	}

	json::Value &credentials = JSONData::_validateKey(info, "credentials", json::ArrayType, true);

	if (credentuals != info) {
		const auto count = credentials.count();

		for (auto i = 0; i < count; ++i) {
			json::Value &entry = credentials[i];

			JSONData::_validateKey(info, "filename", json::StringType);
			JSONData::_validateHash(info, "sha256");
			JSONData::_validateHash(info, "aes256");
		}
	}

	JSONData::_validateKey(info, "image", json::StringType, true);
	JSONData::_validateKey(info, "first name", json::StringType, true);
	JSONData::_validateKey(info, "last name", json::StringType, true);
	JSONData::_validateKey(info, "domain", json::StringType, true);
	JSONData::_validateKey(info, "email", json::StringType, true);
	JSONData::_validateKey(info, "website", json::StringType, true);
	JSONData::_validateKey(info, "twitter", json::StringType, true);
	JSONData::_validateKey(info, "facebook", json::StringType, true);
	JSONData::_validateKey(info, "youtube", json::StringType, true);
	JSONData::_validateKey(info, "country", json::StringType, true);
	JSONData::_validateKey(info, "state", json::StringType, true);
	JSONData::_validateKey(info, "province", json::StringType, true);
	JSONData::_validateKey(info, "city", json::StringType, true);
	JSONData::_validateKey(info, "street", json::StringType, true);
	JSONData::_validateKey(info, "street number", json::StringType, true);
	JSONData::_validateKey(info, "apartment", json::StringType, true);

	JSONData::_validateBase64(wrapper, "signature", true);
	JSONData::_validateHash(wrapper, "signer", true);

	json::Value &verifiers = JSONData::_validateKey(wrapper, "verifiers", json::ArrayType, true);
	const auto verifyingIdentifiers = verifiers.keys();

	for (auto identifier : verifyingIdentifiers) {
		JSONData::_validateKey(verifiers, identifier, json::StringType);
	}
}

inline void PersonalInformation::_changeInfo(json::Value &info,
                                             const dt::DateTime &timestamp,
                                             int matchCount) {
	json::Value wrapper(json::ObjectType);

	info["timestamp"] = int64_t(timestamp - dt::DateTime(2001, dt::DateTime::Jan, 1, dt::DateTime::GMT));
	wrapper["identity"] = info.format();
	_changeContent(wrapper, matchCount);
}

inline void PersonalInformation::_changeContent(json::Value &value, int matchCount) {
	const std::string signer = value["signer"].string();

  do {
    value["padding"] = int64_t(rand());
    JSONData::assign(value, Data::Unencrypted);
  } while (text::matching(Data::identifier(), signer) < matchCount);
  _validate();
}

inline json::Value PersonalInformation::_info(const json::Value &wrapper) {
	return json::Value().parse((wrapper.is(json::NullType) ? JSONData::value() : wrapper)["identity"]);
}

} // namespace data

#endif // __PersonalInformation_h__
