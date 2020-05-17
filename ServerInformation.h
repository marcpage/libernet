#ifndef __ServerInformation_h__
#define __ServerInformation_h__

#include "libernet/JSONData.h"
#include "libernet/Karma.h"
#include "protocol/JSON.h"
#include <algorithm> // std::copy
#include <string>

namespace data {

class ServerInformation : public JSONData {
public:
  enum Connection { FirstConnection, LastConnection };
  ServerInformation() : JSONData() {
    _changeInfo(json::Value(json::ObjectType));
  }
  ServerInformation(const std::string &data, const std::string &identifier)
      : JSONData(data, identifier) {}
  ServerInformation(const ServerInformation &other) : JSONData(other) {}
  virtual ~WrapperData() {}
  void assign(const std::string &data, const std::string &identifier) {
    Data::assign(data, identifier);
  }
  ServerInformation &operator=(const ServerInformation &other) {
    JSONData::operator=(other);
  }
  bool operator==(const ServerInformation &other) {
    JSONData::operator==(other);
  }
  bool operator!=(const ServerInformation &other) { return !(*this == other); }
  std::string name() { return _get("name").string(); }
  void setName(const std::string &newName) { _set("name", newName); }
  std::string owner() { return _get("identifier").string(); }
  void setOwner(const std::string &newIdentifier) {
    _set("identifier", newIdentifier);
  }
  std::string address() { return _get("address").string(); }
  void setAddress(const std::string &newAddress) {
    _set("address", newAddress);
  }
  int port() { return _get("port", json::IntegerType).integer(); }
  void setPort(int newPort) { _set("port", int64_t(newPort)); }
  List &servers(List &identifiers, ListAction action = ClearFirst);
  dt::DateTime connection(const std::string &identifier,
                          Connection which = LastConnection);
  void setConnection(const std::string &identifier,
                     const dt::DateTime &when = dt::DateTime(),
                     Connection which = LastConnection);
  std::string name(const std::string &identifier) {
    return _get(identifier, "name").string();
  }
  void setName(const std::string &identifier, const std::string &newName) {
    _set(identifier, "name", newName);
  }
  std::string address(const std::string &identifier) {
    return _get(identifier, "address").string();
  }
  void setAddress(const std::string &identifier,
                  const std::string &newAddress) {
    _set(identifier, "address", newAddress);
  }
  int port(const std::string &identifier) {
    return _get(identifier, "port", json::IntegerType).integer();
  }
  void setPort(const std::string &identifier, int newPort) {
    _set(identifier, "port", int64_t(newPort));
  }
  int64_t count(const std::string &identifier, const std::string &type) {
    return _get(identifier, type, json::IntegerType).integer();
  }
  void increment(const std::string &identifier, const std::string &type,
                 int64_t amount = 1);

private:
  void _validate();
  json::Value _get(json::Value &value, const std::string &key,
                   json::Type expected = json::StringType);
  json::Value _get(const std::string &key,
                   json::Type expected = json::StringType) {
    return _get(JSONData::value(), key, expected);
  }
  void _set(const std::string &key, const std::string &value);
  void _set(const std::string &key, int64_t value);
  json::Value _get(const std::string &identifier, const std::string &key,
                   json::Type expected = json::StringType);
  void _set(const std::string &identifier, const std::string &key,
            const std::string &value);
  void _set(const std::string &identifier, const std::string &key,
            int64_t value);
};

} // namespace data

inline JSONData::List &
ServerInformation::servers(JSONData::List &identifiers,
                           JSONData::ListAction action = ClearFirst) {
  auto keys = JSONData::value().keys();
  if (ClearFirst == action) {
    identifiers.clear();
  }
  std::copy(keys.begin(), keys.end(), std::back_inserter(identifiers));
  return identifiers;
}

inline dt::DateTime
ServerInformation::connection(const std::string &identifier,
                              Connection which = LastConnection) {
  int64_t timestamp =
      _get(identifier, LastConnection == which ? "latest" : "first",
           json::IntegerType)
          .integer();
  return dt::DateTime(2001, dt::DateTime::Jan, 1, dt::DateTime::GMT) +
         double(timestamp);
}

inline void
ServerInformation::setConnection(const std::string &identifier,
                                 const dt::DateTime &when = dt::DateTime(),
                                 Connection which = LastConnection) {
  int64_t timestamp = int64_t(
      when - dt::DateTime(2001, dt::DateTime::Jan, 1, dt::DateTime::GMT));
  _set(identifier, LastConnection == which ? "latest" : "first", timestamp);
}

inline void ServerInformation::increment(const std::string &identifier,
                                         const std::string &type,
                                         int64_t amount) {
  auto info = JSONData::value();
  json::Value &servers = info["servers"];

  if (!servers.has(identifier)) {
    servers[identifier].makeObject();
  }

  json::Value &entry = servers[identifier];

  if (!entry.has(type)) {
    entry[type] = 0;
  }

  entry[type] += amount;
  JSONData::assign(info, Data::Unencrypted);
}

inline void ServerInformation::_validate() {
  auto info = JSONData::value();

  JSONData::_validateOptionalHash(info, "identifier");
  JSONData::_validateOptionalKey(info, "name", json::StringType);
  JSONData::_validateOptionalKey(info, "address", json::StringType);
  JSONData::_validateOptionalPositiveInteger(info, "port");

  json::Value &servers =
      JSONData::_validateOptionalKey(info, "servers", json::ObjectType);
  auto identifiers = servers.keys();

  for (auto identifier : identifiers) {
    json::Value &entry =
        JSONData::_validateKey(servers, identifier, json::ObjectType);

    JSONData::_validateOptionalKey(entry, "name", json::StringType);
    JSONData::_validateOptionalKey(entry, "adddress", json::StringType);
    JSONData::_validateOptionalPositiveInteger(entry, "port");
    JSONData::_validateOptionalPositiveInteger(entry, "first");
    JSONData::_validateOptionalPositiveInteger(entry, "latest");
    JSONData::_validateOptionalPositiveInteger(entry, "connections");
    JSONData::_validateOptionalPositiveInteger(entry, "failed");
    JSONData::_validateOptionalPositiveInteger(entry, "time");
    JSONData::_validateOptionalPositiveInteger(entry, "input");
    JSONData::_validateOptionalPositiveInteger(entry, "output");
    JSONData::_validateOptionalPositiveInteger(entry, "response");
    JSONData::_validateOptionalPositiveInteger(entry, "similar");

    auto received =
        JSONData::_validateOptionalKey(entry, "received", json::StringType);

    if (karma != entry) {
      karma::Karma(received);
    }
  }
}

inline json::Value
ServerInformation::_get(json::Value &value, const std::string &key,
                        json::Type expected = json::StringType) {
  if (!value.has(key)) {
    return json::Value(expected);
  }
  return value[key];
}

inline void ServerInformation::_set(const std::string &key,
                                    const std::string &value) {
  auto info = JSONData::value();

  info[key] = value;
  JSONData::assign(info, Data::Unencrypted);
}

inline void ServerInformation::_set(const std::string &key, int64_t value) {
  auto info = JSONData::value();

  info[key] = value;
  JSONData::assign(info, Data::Unencrypted);
}

inline json::Value
ServerInformation::_get(const std::string &identifier, const std::string &key,
                        json::Type expected = json::StringType) {
  auto info = JSONData::value();

  if (!info.has("servers")) {
    return json::Value(expected);
  }
  return _get(info["servers"], key, expected);
}

inline void ServerInformation::_set(const std::string &identifier,
                                    const std::string &key,
                                    const std::string &value) {
  auto info = JSONData::value();

  if (!info.has("servers")) {
    info["servers"].makeObject();
  }
  info[key] = value;
  JSONData::assign(info, Data::Unencrypted);
}

inline void ServerInformation::_set(const std::string &identifier,
                                    const std::string &key, int64_t value) {
  auto info = JSONData::value();

  if (!info.has("servers")) {
    info["servers"].makeObject();
  }
  info[key] = value;
  JSONData::assign(info, Data::Unencrypted);
}

#endif // __ServerInformation_h__
