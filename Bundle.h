#ifndef __Bundle_h__
#define __Bundle_h__

#include "libernet/Data.h"
#include "libernet/JSONData.h"
#include "libernet/LargeFile.h"
#include "libernet/MimeTypes.h"
#include "libernet/SmallFile.h"
#include "os/Exception.h"
#include "os/File.h"
#include "os/Hash.h"
#include "os/Path.h"
#include "os/Queue.h"
#include <algorithm>
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
  virtual ~Bundle() {}
  Bundle &assign(const io::Path &p, Queue &q, const List &previous = List());
  Bundle &assign(const std::string &data, const std::string &identifier,
                 const std::string &key);
  Bundle &operator=(const Bundle &other) {
    JSONData::operator=(other);
    return *this;
  }
  bool operator==(const Data &other) const { return Data::operator==(other); }
  bool operator!=(const Data &other) const { return !(*this == other); }
  List &objects(List &dataList);
  bool update(Data &chunk);
  bool write(const io::Path &path, Data &chunk);
  bool operator==(const io::Path &other);
  bool operator!=(const io::Path &other) { return !(*this == other); }
  // TODO: Make flush and reset virtual and override them here to clear cache
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
  void _changeContent(const json::Value &json);
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
  for (auto i = previous.begin(); i != previous.end(); ++i) {
    entry = *i;
    previousList.append(entry);
  }

  json::Value &contents = bundle["contents"] = json::Value(json::ObjectType);
  _listRelative(p, files);
  for (auto i = files.begin(); i != files.end(); ++i) {
    io::Path absolute = p + *i;
    json::Value &fileEntry = contents[std::string(*i)] =
        json::Value(json::ObjectType);
    std::string mimeType = mime::fromExtension(io::Path(*i).extension());
    Data fileData;

    if (absolute.size() <= data_Data_MAX_CONTENTS_SIZE) {
      fileData = SmallFile(absolute);
    } else {
      fileData = _largeFileCache[*i] = LargeFile(absolute, q);
    }

    fileEntry["sha256"] = fileData.identifier();
    fileEntry["aes256"] = fileData.key();
    fileEntry["size"] = static_cast<int64_t>(absolute.size());
    if (mimeType.size() > 0) {
      fileEntry["Content-Type"] = mimeType;
    }
    q.enqueue(fileData);
  }
  _changeContent(bundle);
  return *this;
}

inline Bundle &Bundle::assign(const std::string &data,
                              const std::string &identifier,
                              const std::string &key) {
  Data::assign(data, identifier, key);
  _validate();
  return *this;
}

inline Bundle::List &Bundle::objects(Bundle::List &dataList) {
  auto parsed = JSONData::value();
  auto &contents = parsed["contents"];
  auto keys = contents.keys();

  for (auto name = keys.begin(); name != keys.end(); ++name) {
    dataList.push_back(contents[*name]["sha256"].string());
  }

  for (auto entry = _largeFileCache.begin(); entry != _largeFileCache.end();
       ++entry) {
    entry->second.objects(dataList);
  }
  return dataList;
}

inline bool Bundle::update(Data &chunk) {
  auto parsed = JSONData::value();
  auto &contents = parsed["contents"];
  auto keys = contents.keys();
  auto changed = false;

  for (auto name = keys.begin(); name != keys.end(); ++name) {
    std::string identifier = contents[*name]["sha256"].string();

    if (identifier == chunk.identifier()) {
      try {
        LargeFile file(chunk.data(), chunk.identifier(), chunk.key());

        _largeFileCache[*name] = file;
        changed = true;
      } catch (const msg::Exception &exception) {
      }
    }
  }
  return changed;
}

inline bool Bundle::write(const io::Path &path, Data &chunk) {
  auto parsed = JSONData::value();
  auto &contents = parsed["contents"];
  auto keys = contents.keys();
  auto changed = false;

  if (!path.isDirectory()) {
    path.mkdirs();
  }

  for (auto name = keys.begin(); name != keys.end(); ++name) {
    std::string identifier = contents[*name]["sha256"].string();

    if (identifier == chunk.identifier()) {
      try {
        LargeFile file(chunk.data(), chunk.identifier(), chunk.key());

        _largeFileCache[*name] = file;
        changed = true;
      } catch (const msg::Exception &exception) {
        io::Path filePath = path + *name;
        io::Path directory = filePath.parent();

        SmallFile file(chunk.data(), chunk.identifier(), chunk.key());

        if (!directory.isDirectory()) {
          directory.mkdirs();
        }

        file.write(filePath);
        changed = true;
      }
    }
  }

  for (auto entry = _largeFileCache.begin(); entry != _largeFileCache.end();
       ++entry) {
    changed = entry->second.write(path + entry->first, chunk) | changed;
  }

  return changed;
}

/** Check if this bundle matches a directory on disk.
        @param other The path to a directory to check
        @return true if the bundle is exactly represented on disk.
                        false if we don't have enough information (not all large
   file data chunks have been received) or the files in the directory and the
   files in the bundle do not match.
*/
inline bool Bundle::operator==(const io::Path &other) {
  io::Path::StringList existingFiles;
  auto parsed = JSONData::value();
  auto &contents = parsed["contents"];
  const auto count = contents.count();

  _listRelative(other, existingFiles);
  if (count != static_cast<decltype(count)>(existingFiles.size())) {
    return false;
  }

  for (auto name = existingFiles.begin(); name != existingFiles.end(); ++name) {
    if (!contents.has(*name)) {
      return false;
    }
  }

  json::Value::StringList keys = contents.keys();

  for (auto name = keys.begin(); name != keys.end(); ++name) {
    if (std::find(existingFiles.begin(), existingFiles.end(), *name) ==
        existingFiles.end()) {
      return false;
    }
  }

  for (auto entry = _largeFileCache.begin(); entry != _largeFileCache.end();
       ++entry) {
    if (entry->second != other + entry->first) {
      return false;
    }
  }

  for (auto name = keys.begin(); name != keys.end(); ++name) {
    if (_largeFileCache.find(*name) == _largeFileCache.end()) {
      try {
        if (SmallFile(other + *name).key() !=
            contents[*name]["aes256"].string()) {
          return false;
        }
      } catch (const msg::Exception &) {
        return false;
      }
    }
  }

  return true;
}

inline void Bundle::_validate() {
  json::Value parsed = JSONData::value();

  AssertMessageException(parsed.is(json::ObjectType));
  AssertMessageException(parsed.count() >= 1);
  json::Value &contents =
      JSONData::_validateKey(parsed, "contents", json::ObjectType);
  json::Value::StringList keys = contents.keys();

  for (json::Value::StringList::iterator name = keys.begin();
       name != keys.end(); ++name) {
    json::Value &entry = contents[*name];

    JSONData::_validateHash(entry, "sha256");
    JSONData::_validateHash(entry, "aes256");
    JSONData::_validatePositiveInteger(entry, "size");
    AssertMessageException(!entry.has("Content-Type") ||
                           entry["Content-Type"].is(json::StringType));
  }

  if (parsed.has("previous")) {
    json::Value &previous =
        JSONData::_validateKey(parsed, "previous", json::ArrayType);
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
  return list;
}

inline void Bundle::_changeContent(const json::Value &json) {
  JSONData::assign(json, Data::Encrypted);
  _validate();
}

} // namespace data

#endif // __Bundle_h__
