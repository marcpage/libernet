#ifndef __SimilarResults_h__
#define __SimilarResults_h__

#include "libernet/JSONData.h"
#include "protocol/JSON.h"
#include <algorithm> // std::copy
#include <string>

namespace data {

class SimilarResults : public JSONData {
public:
  SimilarResults() : JSONData() { _changeInfo(json::Value(json::ObjectType)); }
  SimilarResults(const std::string &data, const std::string &identifier)
      : JSONData(data, identifier) {}
  SimilarResults(const SimilarResults &other) : JSONData(other) {}
  virtual ~SimilarResults() {}
  void assign(const std::string &data, const std::string &identifier) {
    Data::assign(data, identifier);
  }
  SimilarResults &operator=(const SimilarResults &other) {
    JSONData::operator=(other);
    return *this;
  }
  bool operator==(SimilarResults &other) { return JSONData::operator==(other); }
  bool operator!=(SimilarResults &other) { return !(*this == other); }
  List &searches(List &list, ListAction action = ClearFirst);
  List &results(const std::string &search, List &list,
                ListAction action = ClearFirst);
  int size(const std::string &search, const std::string &identifier);
  void add(const std::string &search, const std::string &identifier, int size);
  void remove(const std::string &search);
  void remove(const std::string &search, const std::string &identifier);

private:
  void _validate();
  void _changeInfo(const json::Value &value);
};

inline JSONData::List &SimilarResults::searches(JSONData::List &list,
                                                JSONData::ListAction action) {
  auto searchingFor = JSONData::value().keys();

  if (ClearFirst == action) {
    list.clear();
  }
  std::copy(searchingFor.begin(), searchingFor.end(), std::back_inserter(list));
  return list;
}

inline JSONData::List &SimilarResults::results(const std::string &search,
                                               JSONData::List &list,
                                               JSONData::ListAction action) {
  auto info = JSONData::value();

  if (ClearFirst == action) {
    list.clear();
  }

  if (!info.has(search)) {
    return list;
  }

  auto found = info[search].keys();

  std::copy(found.begin(), found.end(), std::back_inserter(list));
  return list;
}

inline int SimilarResults::size(const std::string &search,
                                const std::string &identifier) {
  auto info = JSONData::value();

  if (!info.has(search)) {
    return -1;
  }

  json::Value &results = info[search];

  if (!results.has(identifier)) {
    return -1;
  }

  return results[identifier].integer();
}

inline void SimilarResults::add(const std::string &search,
                                const std::string &identifier, int size) {
  auto info = JSONData::value();

  info[search][identifier] = size;
  _changeInfo(info);
}

inline void SimilarResults::remove(const std::string &search) {
  auto info = JSONData::value();

  if (info.has(search)) {
    info.erase(search);
    _changeInfo(info);
  }
}

inline void SimilarResults::remove(const std::string &search,
                                   const std::string &identifier) {
  auto info = JSONData::value();

  if (info.has(search)) {
    json::Value &found = info[search];

    if (found.has(identifier)) {
      found.erase(identifier);
      _changeInfo(info);
    }
  }
}

inline void SimilarResults::_changeInfo(const json::Value &value) {
  JSONData::assign(value, Data::Unencrypted);
  _validate();
}

inline void SimilarResults::_validate() {
  auto info = JSONData::value();
  auto searches = info.keys();

  for (auto searchTerm : searches) {
    json::Value &results = info[searchTerm];
    auto found = results.keys();

    for (auto identifier : found) {
      JSONData::_validatePositiveInteger(results, identifier);
    }
  }
}

} // namespace data

#endif // __SimilarResults_h__
