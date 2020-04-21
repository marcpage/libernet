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
  Bundle() : JSONData(), _largeFileCache() {}
  Bundle(const io::Path &p, Queue &q, const List &v = List())
      : JSONData(), _largeFileCache() {
    assign(p, q, v);
  }
  Bundle(const std::string &data, const std::string &identifier,
         const std::string &key);
  Bundle(const Bundle &other) : JSONData(other), _largeFileCache() {}
  virtual Bundle() {}
  Bundle &assign(const io::Path &p, Queue &q, const List &previous = List());
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
  // TODO: add setMimeType(relativePath, mimeType)
  // TODO: add listFiles()
  // TODO: add addFile(pathToFile, relativePath, mimeType="")
  // TODO: add removeFile(relativePath)
  // TODO: add has(relativePath)
  // TODO: add write(relativePath, absolutePath)
  // TODO: add addPrevious
  // TODO: add removePrevious
  // TODO: add hasPrevious
private:
  typedef std::map<std::string, LargeFile> LargeFiles;
  mutable LargeFiles _largeFileCache;
  void _validate();
  io::Path::StringList &_listRelative(const io::Path &path,
                                      io::Path::StringList &list);
};

inline Bundle::Bundle(const std::string &data, const std::string &identifier,
                      const std::string &key)
    : JSONData(data, identifier, key), _largeFileCache() {
  _validate();
}

inline Bundle &Bundle::assign(const io::Path &p, Queue &q,
                              const List &previous) {
  io::Path::StringList files;
  json::Value bundle(json::ObjectType);
  json::Value entry;

  json::Value &previousList = bundle["previous"] = json::Value(json::ArrayType);
  for (List::iterator i = previous.begin(); i != previous.end(); ++i) {
    entry = *i;
    previousList.append(entry);
  }

  json::Value &contents = bundle["contents"] = json::Value(json::ObjectType);
  _listRelative(p, files);
  for (io::Path::StringList::iterator i = files.begin(); i != files.end; ++i) {
    io::Path absolute = p + *i;
    Data fileData = loadFile(absolute, q);
    json::Value &fileEntry = contents[std::string(*i)] =
        json::Value(json::ObjectType);
    std::string mimeType = mime::fromExtension(i->extension());

    fileEntry["sha256"] = fileData.identity();
    fileEntry["aes256"] = fileData.key();
    fileEntry["size"] = static_cast<int64_t>(absolute.size());
    if (mimeType.size() > 0) {
      fileEntry["Content-Type"] = mimeType;
    }
    queue.put(fileData);
  }
  _validate();
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
inline void Bundle::_validate() {
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
    AssertMessageException(!entry.has("Content-Type") ||
                           entry["Content-Type"].is(json::StringType))
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

inline io::Path::StringList &Bundle::_listRelative(const io::Path &path,
                                                   io::Path::StringList &list) {
  std::vector<io::Path> directories;
  io::Path::StringList names;

  directories.push_back(io::Path());
  while (directories.size() > 0) {
    io::Path relativeBase = directories[0];
    io::Path directory = path + relativeBase;

    directories.erase(directories.begin());
    directory.list(io::Path::NameOnly, names, io::Path::FlatListing);
    for (io::Path::StringList::iterator name = names.begin();
         name != names.end(); ++name) {
      io::Path relative = relativeBase + *name;
      io::Path absolute = path + relative;

      if (absolute.isDirectory()) {
        directories.push_back(relative);
      } else {
        list.push_back(relative);
      }
    }
  }
}

} // namespace data

#endif // __Bundle_h__
