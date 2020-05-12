#ifndef __AddressHistory_h__
#define __AddressHistory_h__

#include "libernet/Data.h"
#include "libernet/JSONData.h"
#include "protocol/JSON.h"
#include "os/DateTime.h"
#include "os/Exception.h"
#include "os/Hash.h"
#include "os/Text.h"
#include <vector>
#include <string>
#include <stdlib.h> // rand
#include <algorithm> // std::replace

namespace data {

class AddressHistory : public JSONData {
public:
	typedef std::vector<std::string> List;
	AddressHistory():JSONData(),_address() {}
	AddressHistory(const std::string &data, const std::string &identifier,
                      const std::string &key, const std::string &address);
	AddressHistory(const AddressHistory &other):JSONData(other), _address(other._addresss) {}
	virtual ~AddressHistory() {}
	AddressHistory &operator=(const AddressHistory &other);
	AddressHistory &assign(const std::string &data, const std::string &identifier,
                      const std::string &key, const std::string &address);
	const std::string &address() const { return _address; }
	AddressHistory &setAddress(const std::string &address);
	AddressHistory &calculate(int matchCount);
	int match();
	List &bundles(List &list);
	int bundleCount() { return JSONData::value()["heads"].count(); }
	void append(const data::Data &data, const dt::DateTime &timestamp=dt::DateTime()) { append(data.identifier(), data.key(), timestamp); }
	void append(const std::string &identifier, const std::string &key, const dt::DateTime &timestamp=dt::DateTime());
	void insert(int before, const data::Data &data, const dt::DateTime &timestamp=dt::DateTime()) {
		insert(before, data.identifier(), data.key(), timestamp);
	}
	void insert(int before, const std::string &identifier, const std::string &key, const dt::DateTime &timestamp=dt::DateTime());
	void insert(const std::string &beforeIdentifier, const data::Data &data, const dt::DateTime &timestamp=dt::DateTime()) {
		insert(_index(beforeIdentifier), data.identifier(), data.key(), timestamp);
	}
	void insert(const std::string &beforeIdentifiers, const std::string &identifier, const std::string &key, const dt::DateTime &timestamp=dt::DateTime()) {
		insert(_index(beforeIdentifier), identifier, key, timestamp);
	}
	void remove(int index);
	void remove(const std::string &identifier) { remove(_index(identifier)); }
	void sign(int index, const std::string &signer, const std::string &signature);
	void sign(const std::string &bundleIdentifier, const std::string &signer, const std::string &signature) {sign(_index(bundleIdentifier), blocker, signature);}
	void block(int index, const std::string &blocker, const std::string &signature, const std::string &reason);
	void block(const std::string &bundleIdentifier, const std::string &blocker, const std::string &signature, const std::string &reason) {
		block(_index(bundleIdentifier), blocker, signature, reason);
	}
	std::string key(int index);
	std::string key(const std::string &identifier) { return key(_index(identifier)); }
	dt::DateTime timestamp(int index);
	dt::DateTime timestamp(const std::string &identifier) { return timestamp(_index(identifier)); }
	int signatureCount(int index);
	int signatureCount(const std::string &identifier) { return signatureCount(_index(identifier)); }
	List &signers(int index, List &identities);
	List &signers(const std::string &bundleIdentifier, List &identities) { return signers(_index(bundleIdentifier), identities); }
	std::string signature(int bundleIndex, const std::string &signer);
	std::string signature(const std::string &bundleIdentifier, const std::string &signer) { return signature(_index(bundleIdentifier), signer); }
	List &blockers(int index, List &identities);
	List &blockers(const std::string &bundleIdentifier, List &identities) { return blockers(_index(bundleIdentifier), identities); }
	std::string blockSignature(int bundleIndex, const std::string &blocker);
	std::string blockSignature(const std::string &bundleIdentifier, const std::string &blocker) { return blockSignature(_index(bundleIdentifier), blocker); }
	std::string blockReason(int bundleIndex, const std::string &blocker);
	std::string blockReason(const std::string &bundleIdentifier, const std::string &blocker) { return blockReason(_index(bundleIdentifier), blocker); }
private:
	std::string _address;
	int _index(const std::string &identifier);
  void _validate();
  void _changeContent(json::Value &value, int matchCount=0);
  std::string _matchAgainst();
};

inline AddressHistory::AddressHistory(const std::string &data, const std::string &identifier,
                      const std::string &key, const std::string &address):JSONData(data, identifier, key):JSONData(data, identifier, key), _address(address) {}

inline AddressHistory &voperator=(const AddressHistory &other) {
	JSONData::operator=(other);
	_address = other._address;
	return *this;
}

inline AddressHistory &AddressHistory::assign(const std::string &data, const std::string &identifier,
                      const std::string &key, const std::string &address) {
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

inline List &AddressHistory::bundles(List &list) {
    json::Value history = JSONData::value();
    json::Value &heads = history["heads"];
    const int count = heads.count();

	for (int i = 0; i < count; ++i) {
		list.push_back(history[i]["sha256"].string());
	}
	return list;
}

inline void AddressHistory::append(const std::string &identifier, const std::string &key, const dt::DateTime &timestamp=dt::DateTime()) {
    json::Value history = JSONData::value();
    json::Value &heads = history["heads"];
    json::Value entry(json::ObjectType);
    entry["sha256"] = identifier;
    entry["aes256"] = key;
    entry['timestamp'] = int64_t(timestamp - dt::DateTime(2001, dt::DateTime::Jan, 1, dt::DateTime::GMT));
    entry["signed"] = json::Object(json::ObjectType);
    entry["blocked"] = json::Object(json::ObjectType);
    heads.append(entry);
	_changeContent(history);
}

inline void AddressHistory::insert(int before, const std::string &identifier, const std::string &key, const dt::DateTime &timestamp=dt::DateTime()) {
    json::Value history = JSONData::value();
    json::Value &heads = history["heads"];
    json::Value entry(json::ObjectType);
    entry["sha256"] = identifier;
    entry["aes256"] = key;
    entry['timestamp'] = int64_t(timestamp - dt::DateTime(2001, dt::DateTime::Jan, 1, dt::DateTime::GMT));
    entry["signed"] = json::Object(json::ObjectType);
    entry["blocked"] = json::Object(json::ObjectType);
    heads.insert(entry, before);
	_changeContent(history);
}

inline void AddressHistory::remove(int index) {
    json::Value history = JSONData::value();
    json::Value &heads = history["heads"];

    heads.erase(index, index + 1);
	_changeContent(history);
}

inline void AddressHistory::sign(int index, const std::string &signer, const std::string &signature) {
    json::Value history = JSONData::value();
    json::Value &signers = history["heads"][index]["signed"];

    signers[signer] = signature;
	_changeContent(history);
}

inline void AddressHistory::block(int index, const std::string &signer, const std::string &signature, const std::string &reason) {
    json::Value history = JSONData::value();
    json::Value &blockers = history["heads"][index]["blocked"];
	json::Value entry(json::ObjectType);

	entry["signed"] = signature;
	entry["reason"] = reason;
    blockers[signer] = entry;

	_changeContent(history);
}

inline std::string AddressHistory::key(int index) {
	return JSONData::value()["heads"][index]["key"].string();
}

inline dt::DateTime AddressHistory::timestamp(int index) {
	return dt::DateTime(2001, dt::DateTime::Jan, 1, dt::DateTime::GMT) + double(JSONData::value()["heads"][index]["timestamp"].integer());
}

inline int AddressHistory::signatureCount(int index) {
	return JSONData::value()["heads"][index]["signed"].count();
}

inline List &AddressHistory::signers(int index, List &identities) {
    auto signerList = JSONData::value()["heads"][index]["signed"].keys();

    for (auto identity : signerList) {
    	identities.push_back(identity);
    }
    return identities;
}

inline std::string AddressHistory::signature(int bundleIndex, const std::string &signer) {
	return JSONData::value()["heads"][index]["signed"][signer].string();
}

inline List &AddressHistory::blockers(int index, List &identities) {
    auto signerList = JSONData::value()["heads"][index]["blocked"].keys();

    for (auto identity : signerList) {
    	identities.push_back(identity);
    }
    return identities;
}

inline std::string AddressHistory::blockSignature(int bundleIndex, const std::string &blocker) {
	return JSONData::value()["heads"][index]["signed"][blocker]["signed"].string();
}

inline std::string AddressHistory::blockReason(int bundleIndex, const std::string &blocker) {
	return JSONData::value()["heads"][index]["signed"][blocker]["reason"].string();
}

inline int AddressHistory::_index(const std::string &identifier) {
	auto identifiers = JSONData::value()["heads"].keys();
	const auto count = identifiers.size();

	for(decltype(count) i = 0; i < count; ++i) {
		if (identifier == identifiers[i]) {
			return int(i);
		}
	}
	ThrowMessageException("bundle identifier not found " + identifier);
}

inline void AddressHistory::_validate() {
    json::Value history = JSONData::value();

	JSONData::_validatePositiveInteger(history, "padding");

	json::Value &heads = JSONData::_validateKey(history, "heads", json::ArrayType);
	const auto count = heads.count();

	for (decltype(count) i = 0; i < count; ++i) {
		json::Value &entry = heads[i];

		AssertMessageException(entry.is(json::ObjectType));
		JSONData::_validateHash(entry, "sha256");
		JSONData::_validateHash(entry, "aes256");
		JSONData::_validatePositiveInteger(entry, "timestamp");

		json::Value &signatures = JSONData::_validateKey(entry, "signed", json::ObjectType);
		json::Value &blocking = JSONData::_validateKey(entry, "blocked", json::ObjectType);
		auto signers = signatures.keys();
		auto blockers = blocking.keys();

		for (auto identity : signers) {
			hash::sha256().reset(identity.string().c_str());
			JSONData::_validateHash(signers, identity);  // TODO validate is signature
		}

		for (auto identity : blockers) {
			hash::sha256().reset(identity.string().c_str());
			json::Value &entry = JSONData::_validateKey(blockers, identity, json::ObjectType);

			JSONData::_validateKey(entry, "reason", json::StringType);
			JSONData::_validateHash(entry, "signed"); // TODO validate is signature
		}
	}
}

inline void AddressHistory::_changeContent(json::Value &value, int matchCount) {
	const std::string match = matchCount == 0 ? std::string() : _matchAgainst();

	do {
		value["padding"] = int64_t(rand());
	  JSONData::assign(json, Data::Encrypted);
	} while(text::matching(Data::identifier(), match) >= matchCount);
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

}

#endif // __AddressHistory_h__
