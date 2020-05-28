#ifndef __AddressHistory_h__
#define __AddressHistory_h__

#include "libernet/Data.h"
#include "libernet/JSONData.h"
#include "os/DateTime.h"
#include "os/Exception.h"
#include "os/Hash.h"
#include "os/Text.h"
#include "protocol/JSON.h"
#include <algorithm> // std::replace, std::copy
#include <stdlib.h>  // rand
#include <string>
#include <vector>

namespace data {

/// @todo Document
/// @todo update to support "password" field in heads and optional aes256
class AddressHistory : public JSONData {
public:
  AddressHistory() : JSONData(), _address() {
    _changeContent(json::Value().parse("{\"heads\":[]}"));
  }
  AddressHistory(const std::string &data, const std::string &identifier,
                 const std::string &key, const std::string &address);
  AddressHistory(const AddressHistory &other)
      : JSONData(other), _address(other._address) {}
  virtual ~AddressHistory() {}
  AddressHistory &operator=(const AddressHistory &other);
  AddressHistory &assign(const std::string &data, const std::string &identifier,
                         const std::string &key, const std::string &address);
  const std::string &address() const { return _address; }
  AddressHistory &setAddress(const std::string &address);
  AddressHistory &calculate(int matchCount);
  int match();
  List &bundles(List &list, ListAction action = ClearFirst);
  int bundleCount() { return JSONData::value()["heads"].count(); }
  void append(data::Data &data, const std::string &username="", const std::string &password="", const dt::DateTime &timestamp = dt::DateTime()) {
    append(data.identifier(), data.key(), username, password, timestamp);
  }
  void append(const std::string &identifier, const std::string &key, const std::string &username="", const std::string &password="", const dt::DateTime &timestamp = dt::DateTime());
  void insert(int before, data::Data &data, const std::string &username="", const std::string &password="", const dt::DateTime &timestamp = dt::DateTime()) {
    insert(before, data.identifier(), data.key(), username, password, timestamp);
  }
  void insert(int before, const std::string &identifier, const std::string &key, const std::string &username="", const std::string &password="", const dt::DateTime &timestamp = dt::DateTime());
  void insert(const std::string &beforeIdentifier, data::Data &data, const std::string &username="", const std::string &password="", const dt::DateTime &timestamp = dt::DateTime()) {
    insert(_index(beforeIdentifier), data.identifier(), data.key(), username, password, timestamp);
  }
  // @todo Test
  void insert(const std::string &beforeIdentifier, const std::string &identifier, const std::string &key, const std::string &username="", const std::string &password="", const dt::DateTime &timestamp = dt::DateTime()) {
    insert(_index(beforeIdentifier), identifier, key, username, password, timestamp);
  }
  void remove(int index);
  void remove(const std::string &identifier) { remove(_index(identifier)); }
  void sign(int index, const std::string &signer, const std::string &signature);
  void sign(const std::string &bundleIdentifier, const std::string &signer,
            const std::string &signature) {
    sign(_index(bundleIdentifier), signer, signature);
  }
  void block(int index, const std::string &blocker,
             const std::string &signature, const std::string &reason);
  void block(const std::string &bundleIdentifier, const std::string &blocker,
             const std::string &signature, const std::string &reason) {
    block(_index(bundleIdentifier), blocker, signature, reason);
  }
  bool hasKey(int index);
  bool hasKey(const std::string &identifier) {
  	return hasKey(_index(identifier));
  }
  bool hasUsername(int index, const std::string &username);
  bool hasUsername(const std::string &identifier, const std::string &username) {
  	return hasUsername(_index(identifier), username);
  }
  std::string key(int index, const std::string &username, const std::string &password);
  std::string key(const std::string &identifier, const std::string &username, const std::string &password) {
  	return key(_index(identifier), username, password);
  }
  std::string key(int index);
  std::string key(const std::string &identifier) {
    return key(_index(identifier));
  }
  dt::DateTime timestamp(int index);
  dt::DateTime timestamp(const std::string &identifier) {
    return timestamp(_index(identifier));
  }
  int signatureCount(int index);
  int signatureCount(const std::string &identifier) {
    return signatureCount(_index(identifier));
  }
  List &signers(int index, List &identities, ListAction action = ClearFirst);
  List &signers(const std::string &bundleIdentifier, List &identities,
                ListAction action = ClearFirst) {
    return signers(_index(bundleIdentifier), identities, action);
  }
  std::string signature(int bundleIndex, const std::string &signer);
  std::string signature(const std::string &bundleIdentifier,
                        const std::string &signer) {
    return signature(_index(bundleIdentifier), signer);
  }
  int blockCount(int index);
  int blockCount(const std::string &identifier) {
    return blockCount(_index(identifier));
  }
  List &blockers(int index, List &identities, ListAction action = ClearFirst);
  List &blockers(const std::string &bundleIdentifier, List &identities,
                 ListAction action = ClearFirst) {
    return blockers(_index(bundleIdentifier), identities, action);
  }
  std::string blockSignature(int bundleIndex, const std::string &blocker);
  std::string blockSignature(const std::string &bundleIdentifier,
                             const std::string &blocker) {
    return blockSignature(_index(bundleIdentifier), blocker);
  }
  std::string blockReason(int bundleIndex, const std::string &blocker);
  std::string blockReason(const std::string &bundleIdentifier,
                          const std::string &blocker) {
    return blockReason(_index(bundleIdentifier), blocker);
  }

private:
  std::string _address;
  int _index(const std::string &identifier);
  void _validate();
  void _changeContent(json::Value &value, int matchCount = 0);
  std::string _matchAgainst();
};

inline AddressHistory::AddressHistory(const std::string &data,
                                      const std::string &identifier,
                                      const std::string &key,
                                      const std::string &address)
    : JSONData(data, identifier, key), _address(address) {}

inline AddressHistory &AddressHistory::operator=(const AddressHistory &other) {
  JSONData::operator=(other);
  _address = other._address;
  return *this;
}

inline AddressHistory &AddressHistory::assign(const std::string &data,
                                              const std::string &identifier,
                                              const std::string &key,
                                              const std::string &address) {
  Data::assign(data, identifier, key);
  _address = address;
  _validate();
  return *this;
}

inline AddressHistory &AddressHistory::setAddress(const std::string &address) {
  json::Value history = JSONData::value();
  _address = address;
  _changeContent(history);
  return *this;
}

inline AddressHistory &AddressHistory::calculate(int matchCount) {
  json::Value history = JSONData::value();
  _changeContent(history, matchCount);
  return *this;
}

int AddressHistory::match() {
  return text::matching(Data::identifier(), _matchAgainst());
}

inline JSONData::List &AddressHistory::bundles(JSONData::List &list,
                                               ListAction action) {
  json::Value history = JSONData::value();
  json::Value &heads = history["heads"];
  const int count = heads.count();

  if (ClearFirst == action) {
    list.clear();
  }
  for (int i = 0; i < count; ++i) {
    list.push_back(heads[i]["sha256"].string());
  }
  return list;
}

inline void AddressHistory::append(const std::string &identifier,
                                   const std::string &key,
                                   const dt::DateTime &timestamp) {
  json::Value history = JSONData::value();
  json::Value &heads = history["heads"];
  json::Value entry(json::ObjectType);
  entry["sha256"] = identifier;
  entry["aes256"] = key;
  entry["timestamp"] = int64_t(
      timestamp - dt::DateTime(2001, dt::DateTime::Jan, 1, dt::DateTime::GMT));
  entry["signed"] = json::Value(json::ObjectType);
  entry["blocked"] = json::Value(json::ObjectType);
  heads.append(entry);
  _changeContent(history);
}

inline void AddressHistory::insert(int before, const std::string &identifier,
                                   const std::string &key,
                                   const dt::DateTime &timestamp) {
  json::Value history = JSONData::value();
  json::Value &heads = history["heads"];
  json::Value entry(json::ObjectType);
  entry["sha256"] = identifier;
  entry["aes256"] = key;
  entry["timestamp"] = int64_t(
      timestamp - dt::DateTime(2001, dt::DateTime::Jan, 1, dt::DateTime::GMT));
  entry["signed"] = json::Value(json::ObjectType);
  entry["blocked"] = json::Value(json::ObjectType);
  heads.insert(entry, before);
  _changeContent(history);
}

inline void AddressHistory::remove(int index) {
  json::Value history = JSONData::value();
  json::Value &heads = history["heads"];

  heads.erase(index, index + 1);
  _changeContent(history);
}

inline void AddressHistory::sign(int index, const std::string &signer,
                                 const std::string &signature) {
  json::Value history = JSONData::value();
  json::Value &signers = history["heads"][index]["signed"];

  signers[signer] = signature;
  _changeContent(history);
}

inline void AddressHistory::block(int index, const std::string &signer,
                                  const std::string &signature,
                                  const std::string &reason) {
  json::Value history = JSONData::value();
  json::Value &blockers = history["heads"][index]["blocked"];
  json::Value entry(json::ObjectType);

  entry["signed"] = signature;
  entry["reason"] = reason;
  blockers[signer] = entry;

  _changeContent(history);
}

inline bool AddressHistory::hasKey(int index) {
  return JSONData::value()["heads"][index].has("aes256");
}

inline bool AddressHistory::hasUsername(int index, const std::string &username) {
	const json::Value history = JSONData::value();
	const json::Value &entry = history["heads"][index];

  if (entry.has("password")) {
  	return entry["password"].has(hash::sha256(username).base64());
  }
  return false;
}

inline std::string AddressHistory::key(int index, const std::string &username, const std::string &password) {
	std::string usernameKey = hash::sha256(username).base64();
    std::string encryptedKey = text::base64Decode(JSONData::value()["heads"][index][usernameKey].string());
	hash::sha256 keyData(username + ":" + password);
    crypto::AES256 key(reinterpret_cast<const char *>(keyData.buffer()), keyData.size());
    std::string decryptedKey;

    key.crypto::SymmetricKey::decryptInPlace(encryptedKey, "", decryptedKey);
    return decryptedKey;
}

inline std::string AddressHistory::key(int index) {
  return JSONData::value()["heads"][index]["aes256"].string();
}

inline dt::DateTime AddressHistory::timestamp(int index) {
  return dt::DateTime(2001, dt::DateTime::Jan, 1, dt::DateTime::GMT) +
         double(JSONData::value()["heads"][index]["timestamp"].integer());
}

inline int AddressHistory::signatureCount(int index) {
  return JSONData::value()["heads"][index]["signed"].count();
}

inline JSONData::List &AddressHistory::signers(int index,
                                               JSONData::List &identities,
                                               ListAction action) {
  auto signerList = JSONData::value()["heads"][index]["signed"].keys();

  if (ClearFirst == action) {
    identities.clear();
  }
  std::copy(signerList.begin(), signerList.end(),
            std::back_inserter(identities));
  // for (auto identity : signerList) {identities.push_back(identity);}
  return identities;
}

inline std::string AddressHistory::signature(int bundleIndex,
                                             const std::string &signer) {
  return JSONData::value()["heads"][bundleIndex]["signed"][signer].string();
}

inline int AddressHistory::blockCount(int index) {
  return JSONData::value()["heads"][index]["blocked"].count();
}

inline JSONData::List &AddressHistory::blockers(int index,
                                                JSONData::List &identities,
                                                ListAction action) {
  auto signerList = JSONData::value()["heads"][index]["blocked"].keys();

  if (ClearFirst == action) {
    identities.clear();
  }
  std::copy(signerList.begin(), signerList.end(),
            std::back_inserter(identities));
  // for (auto identity : signerList) {identities.push_back(identity);}
  return identities;
}

inline std::string AddressHistory::blockSignature(int bundleIndex,
                                                  const std::string &blocker) {
  return JSONData::value()["heads"][bundleIndex]["blocked"][blocker]["signed"]
      .string();
}

inline std::string AddressHistory::blockReason(int bundleIndex,
                                               const std::string &blocker) {
  return JSONData::value()["heads"][bundleIndex]["blocked"][blocker]["reason"]
      .string();
}

inline int AddressHistory::_index(const std::string &identifier) {
  json::Value history = JSONData::value();
  json::Value &heads = history["heads"];
  const auto count = int(heads.count());

  for (auto i = 0; i < count; ++i) {
    if (identifier == heads[i]["sha256"].string()) {
      return int(i);
    }
  }
  ThrowMessageException("bundle identifier not found " + // not tested
                        identifier);
}

inline void AddressHistory::_validate() {
  json::Value history = JSONData::value();

  JSONData::_validatePositiveInteger(history, "padding");

  json::Value &heads =
      JSONData::_validateKey(history, "heads", json::ArrayType);
  const auto count = int(heads.count());

  for (auto i = 0; i < count; ++i) {
    json::Value &entry = heads[i];

    AssertMessageException(entry.is(json::ObjectType));
    JSONData::_validateHash(entry, "sha256");
    JSONData::_validateHash(entry, "aes256");
    JSONData::_validatePositiveInteger(entry, "timestamp");

    json::Value &signatures =
        JSONData::_validateKey(entry, "signed", json::ObjectType);
    json::Value &blocking =
        JSONData::_validateKey(entry, "blocked", json::ObjectType);
    auto signers = signatures.keys();
    auto blockers = blocking.keys();

    for (auto identity : signers) {
      hash::sha256().reset(identity.c_str());
      JSONData::_validateBase64(signatures, identity);
    }

    for (auto identity : blockers) {
      hash::sha256().reset(identity.c_str());
      json::Value &blockingEntry =
          JSONData::_validateKey(blocking, identity, json::ObjectType);

      JSONData::_validateKey(blockingEntry, "reason", json::StringType);
      text::base64Decode(
          JSONData::_validateKey(blockingEntry, "signed", json::StringType)
              .string());
    }
  }
}

inline void AddressHistory::_changeContent(json::Value &value, int matchCount) {
  const std::string match = matchCount == 0 ? std::string() : _matchAgainst();

  // TODO performance improvement, have padding be a unique pattern
  // search and replace that pattern in the string and assign the string
  // data instead of JSONData assign
  do {
    value["padding"] = int64_t(rand());
    JSONData::assign(value, Data::Encrypted);
  } while (text::matching(Data::identifier(), match) < matchCount);
  _validate();
}

inline std::string AddressHistory::_matchAgainst() {
  std::string address;

  text::tolower(_address, address);
  std::replace(address.begin(), address.end(), '\\', '/');
  text::trim(address, ' ');
  text::trim(address, '\t');
  text::trim(address, '\r');
  text::trim(address, '\f');
  text::trim(address, '/');
  text::trim(address, ' ');
  text::trim(address, '\t');
  text::trim(address, '\r');
  text::trim(address, '\f');
  return hash::sha256(address).hex();
}

} // namespace data

#endif // __AddressHistory_h__
