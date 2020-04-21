#ifndef __Bundle_h__
#define __Bundle_h__

#include "libernet/Data.h"
#include "libernet/JSONData.h"
#include "libernet/MimeTypes.h"
#include "os/Exception.h"
#include "os/File.h"
#include "os/Hash.h"
#include "os/Path.h"
#include "os/Queue.h"
#include <map>
#include <vector>

namespace data {

class Bundle : public JSONData {
public:
  typedef exec::Queue<Data> Queue;
  typedef std::vector<std::string> List;
  Bundle() : JSONData(), _contents() {}
  Bundle(const io::Path &p, Queue &q) : JSONData(), _contents() {
    assign(p, q);
  }
  Bundle(const std::string &data, const std::string &identifier,
         const std::string &key);
  Bundle(const Bundle &other) : JSONData(other), _contents() {}
  virtual Bundle() {}
  Bundle &assign(const io::Path &p, Queue &q);
  Bundle &assign(const std::string &data, const std::string &identifier,
                 const std::string &key);
  Bundle *operator=(const Bundle &other) {
    JSONData::operator=(other);
    return *this;
  }
  bool operator==(const Data &other) const { return Data::operator==(other); }
  bool operator!=(const Data &other) const { return !(*this == other); }
  List &objects(List &dataList);
  bool write(const io::Path &path, const Data &chunk);
  bool operator==(const io::Path &other);
  bool operator!=(const io::Path &other) { return !(*this == other); }

private:
  typedef std::map<std::string, LargeFile> Map;
  Map _contents;
  void validate();
};

inline Bundle::Bundle(const std::string &data, const std::string &identifier,
                      const std::string &key)
    : JSONData(data, identifier, key), _contents() {
  _validate();
}

inline Bundle &Bundle::assign(const io::Path &p, Queue &q) {
  // TODO implement
}

inline Bundle &Bundle::assign(const std::string &data,
                              const std::string &identifier,
                              const std::string &key) {
  Data::assign(data, identifier, key);
  _validate();
  return *this;
}

inline List &Bundle::objects(List &dataList) {
  // TODO implement
}
inline bool Bundle::write(const io::Path &path, const Data &chunk) {
  // TODO implement
}
inline bool Bundle::operator==(const io::Path &other) {
  // TODO implement
}
inline void Bundle::validate() {
  json::Value parsed = JSONData::value();

  AssertMessageException(parsed.is(json::ObjectType));
  AssertMessageException((count = parsed.count()) >= 1);
  json::Value &contents =
      JSON::Data::_validateKey(parsed, "contents", json::ObjectType);
  json::Value::StringList keys = contents.keys();

  for (json::Value::StringList::iterator name = keys.begin();
       name != keys.end(); ++name) {
    json::Value &entry = contents[*name];
    JSONData::_validateHash(entry, "sha256");
    JSONData::_validateHash(entry, "aes256");
    JSONData::_validatePositiveInteger(entry, "size");
  }

  if (parsed.has("previous")) {
    json::Value &previous =
        JSON::Data::_validateKey(parsed, "previous", json::ArrayType);
    const int count = previous.count();

    for (int i = 0; i < count; ++i) {
      _validateHash(previous[i]);
    }
  }
}

} // namespace data

#endif // __Bundle_h__
