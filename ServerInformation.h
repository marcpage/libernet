#ifndef __ServerInformation_h__
#define __ServerInformation_h__

#include "libernet/JSONData.h"
#include <algorithm> // std::copy
#include <string>
#include "protocol/JSON.h"

namespace data {

class ServerInformation : public JSONData {
public:
enum Connection {FirstConnection, LastConnection};
ServerInformation():JSONData() { _changeInfo(json::Value(json::ObjectType)); }
ServerInformation(const std::string &data, const std::string &identifier):JSONData(data, identifier) {}
ServerInformation(const ServerInformation &other):JSONData(other) {}
virtual ~WrapperData() {}
void assign(const std::string &data, const std::string &identifier) {Data::assign(data, identifier);}
ServerInformation &operator=(const ServerInformation &other) { JSONData::operator=(other); }
bool operator==(const ServerInformation &other) { JSONData::operator==(other); }
bool operator!=(const ServerInformation &other) { return !(*this == other); }
std::string name() { return _get("name").string(); }
void setName(const std::string &newName) { _set("name", newName); }
std::string identifier() { return _get("identifier").string(); }
void setIdentifier(const std::string &newIdentifier) { _set("identifier", newIdentifier); }
std::string address() { return _get("address").string(); }
void setAddress(const std::string &newAddress) { _set("address", newAddress); }
int port() { return _get("port", json::IntegerType).integer(); }
void setPort(int newPort) { _set("port", int64_t(newPort)); }
List &servers(List &identifiers, ListAction action=ClearFirst);
dt::DateTime connection(const std::string &identifier, Connection which=LastConnection);
void setConnection(const std::string &identifier, const dt::DateTime &when=dt::DateTime(), Connection which=LastConnection);
std::string name(const std::string &identifier) { return _get(identifier, "name").string(); }
void setName(const std::string &identifier, const std::string &newName) { _set(identifier, "name", newName); }
std::string address(const std::string &identifier) { return _get(identifier, "address").string(); }
void setAddress(const std::string &identifier, const std::string &newAddress) { _set(identifier, "address", newAddress); }
int port(const std::string &identifier) { return _get(identifier, "port", json::IntegerType).integer(); }
void setPort(const std::string &identifier, int newPort) { _set(identifier, "port", int64_t(newPort)); }
int64_t getCount(const std::string &identifier, const std::string &type) { return _get(identifier, type, json::IntegerType).integer(); }
void increment(const std::string &identifier, const std::string &type);

private:
void _validate();
json::Value _get(json::Value &value, const std::string &key, json::Type expected=json::StringType);
json::Value _get(const std::string &key, json::Type expected=json::StringType) { return _get(JSONData::value(), key, expected); }
void _set(const std::string &key, const std::string &value);
void _set(const std::string &key, int64_t value);
json::Value _get(const std::string &identifier, const std::string &key, json::Type expected=json::StringType);
void _set(const std::string &identifier, const std::string &key, const std::string &value);
void _set(const std::string &identifier, const std::string &key, int64_t value);
};

}

inline JSONData::List &ServerInformation::servers(JSONData::List &identifiers, JSONData::ListAction action=ClearFirst) {
	auto keys = JSONData::value().keys();
	if (ClearFirst == action) {
		identifiers.clear();
	}
std::copy(keys.begin(), keys.end(), std::back_inserter(identifiers));
return identifiers;
}

inline dt::DateTime ServerInformation::connection(const std::string &identifier, Connection which=LastConnection) {
	int64_t timestamp = _get(identifier, LastConnection == which ? "latest" : "first", json::IntegerType).integer();
	  return dt::DateTime(2001, dt::DateTime::Jan, 1, dt::DateTime::GMT) +
         double(timestamp);
}

inline void ServerInformation::setConnection(const std::string &identifier, const dt::DateTime &when=dt::DateTime(), Connection which=LastConnection) {
	int64_t timestamp = int64_t(when - dt::DateTime(2001, dt::DateTime::Jan, 1, dt::DateTime::GMT));
	_set(identifier, LastConnection == which ? "latest" : "first", timestamp);
}

inline void ServerInformation::increment(const std::string &identifier, const std::string &type) {
	auto info = JSONData::value();
	json::Value &servers = info["servers"];

	if (!servers.has(identifier)) {
		servers[identifier].makeObject();
	}

	json::Value &entry = servers[identifier];

	if (!entry.has(type)) {
		entry[type] = 0;
	}

	entry[type] += 1;
    JSONData::assign(info, Data::Unencrypted);
}

inline void ServerInformation::_validate() {
	// todo implement
}

inline json::Value ServerInformation::_get(json::Value &value, const std::string &key, json::Type expected=json::StringType) {
	if (!value.has(key)) {
		return json::Value(expected);
	}
	return value[key];
}

inline void ServerInformation::_set(const std::string &key, const std::string &value) {
	auto info = JSONData::value();

	info[key] = value;
    JSONData::assign(info, Data::Unencrypted);
}

inline void ServerInformation::_set(const std::string &key, int64_t value) {
	auto info = JSONData::value();

	info[key] = value;
    JSONData::assign(info, Data::Unencrypted);
}

inline json::Value ServerInformation::_get(const std::string &identifier, const std::string &key, json::Type expected=json::StringType) {
	auto info = JSONData::value();

	if (!info.has("servers")) {
		return json::Value(expected);
	}
	return _get(info["servers"], key, expected);
}
inline void ServerInformation::_set(const std::string &identifier, const std::string &key, const std::string &value) {
	auto info = JSONData::value();

	if (!info.has("servers")) {
		info["servers"].makeObject();
	}
	info[key] = value;
    JSONData::assign(info, Data::Unencrypted);
}
inline void ServerInformation::_set(const std::string &identifier, const std::string &key, int64_t value) {
	auto info = JSONData::value();

	if (!info.has("servers")) {
		info["servers"].makeObject();
	}
	info[key] = value;
    JSONData::assign(info, Data::Unencrypted);
}

#endif // __ServerInformation_h__

/*
{
	"identifier": hash identifier of the node,
	"name": the name of this node,
	"address": the address of this node,
	"port": the port number this node is listening on,
	"servers": {
		"identifier": {
			"name": the node's name,
			"address": name or ip address,
			"port": port listening on,
			"first": timestamp of first successful connection,
			"latest": timestamp latest successful connection,
			"connections": number of times connected,
			"failed": number of failed connection attempts,
			"time": total time in seconds connected to the server,
			"input": total bytes received from this node,
			"output": total bytes sent to this node,
			"response": total times we received an exact match for what we needed,
			"similar": total items returned for similar-to searchs,
			"karma": karma received from this node,
		}
	},
}*/
