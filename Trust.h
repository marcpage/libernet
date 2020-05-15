#ifndef __Trust_h__
#define __Trust_h__

#include "libernet/WrapperData.h"

namespace data {

/// @todo document
/// @todo add comments to Trust document for identities (also update readme)
class Trust : public WrapperData {
public:
  Trust() : WrapperData("{\"trust\":{}}") {}
  Trust(const Trust &other) : WrapperData(other) {}
  Trust(const std::string &data, const std::string &identifier,
        const std::string &key)
      : WrapperData(data, identifier, key) {}
  virtual ~Trust() {}
  Trust &operator=(const Trust &other);
  Trust &assign(const std::string &data, const std::string &identifier,
                const std::string &key);
  List &identities(List &trusted, ListAction action = ClearFirsts);
  bool has(const std::string &identity) { return _info()["trust"].has(identity); }
  void addTrust(const std::string &identity) { _increment(identity, "trusted"); }
  void addMistaken(const std::string &identity) { _increment(identity, "mistaken"); }
  void addDisagree(const std::string &identity) { _increment(identity, "disagree"); }
  void addMalevolent(const std::string &identity) { _increment(identity, "malevolent"); }
  int trust(const std::string &identity) { return _value(identity, "trusted"); }
  int mistaken(const std::string &identity) { return _value(identity, "mistaken"); }
  int disagree(const std::string &identity) { return _value(identity, "disagree"); }
  int malevolent(const std::string &identity) { return _value(identity, "malevolent"); }

private:
  void _validate() override;
  std::string _wrapKey() override { return "trust"; }
  void _increment(const std::string &identity, const std::string &key);
  int _value(const std::string &identity, const std::string &key);
};

inline Trust &Trust::operator=(const Trust &other) {
  WrapperData::operator=(other);
  return *this;
}

inline Trust &Trust::assign(const std::string &data,
                            const std::string &identifier,
                            const std::string &key) {
  Data::assign(data, identifier, key);
  _validate();
  return *this;
}

inline JSONData::List &Trust::identities(JSONData::List &trusted,
                                         ListAction action) {
	auto keys = _info()["trust"].keys();

  if (ClearFirst == action) {
    trusted.clear();
  }

  std::copy(keys.begin(), keys.end(), std::back_inserter(trusted));
	return trusted;
}

inline void Trust::_validate() {
	auto info = _info();
  json::Value &trust =
      JSONData::_validateOptionalKey(info, "trust", json::ObjectType);

	WrapperData::_validate();
	if (trust != info) {
		auto identifiers = trust.keys();

		for (auto identifier : identifiers) {
			json::Value &entry = trust[identifier];

			JSONData::_validateOptionalPositiveInteger(entry, "trusted");
			JSONData::_validateOptionalPositiveInteger(entry, "mistaken");
			JSONData::_validateOptionalPositiveInteger(entry, "disagree");
			JSONData::_validateOptionalPositiveInteger(entry, "malevolent");
		}
	}
}

inline void Trust::_increment(const std::string &identity, const std::string &key) {
	auto trust = _info();
	json::Value &identityInfo = trust["trust"][identity];
	const int64_t count = identityInfo.has(key) ? identityInfo[key] : 0;

	trusted[identity][key] = count + 1;
	_changeInfo(trust);
}

inline int Trust::_value(const std::string &identity, const std::string &key) {
	auto trust = _info();
	json::Value &identityInfo = trust["trust"][identity];

	return  identityInfo.has(key) ? identityInfo[key].integer() : 0;
}


} // namespace data

#endif // __Trust_h__
