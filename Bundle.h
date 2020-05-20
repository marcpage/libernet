#ifndef __Bundle_h__
#define __Bundle_h__

#include "libernet/Data.h"
#include "libernet/JSONData.h"
#include "libernet/LargeFile.h"
#include "libernet/MimeTypes.h"
#include "libernet/SmallFile.h"
#include "os/DateTime.h"
#include "os/Exception.h"
#include "os/File.h"
#include "os/Hash.h"
#include "os/Path.h"
#include "os/Queue.h"
#include <algorithm>
#include <map>
#include <vector>

namespace data {

/**
        @todo Document
*/
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
  double timestamp() { return JSONData::value()["timestamp"].integer(); }
  std::string comment() {
    json::Value bundle = JSONData::value();
    if (bundle.has("comments")) {
      return bundle["comments"].string();
    }
    return ""; // not tested
  }
  void resetComment(const std::string &comment = "");
  List &files(List &fileList);
  List files() {
    List buffer;
    return files(buffer);
  }
  bool hasFile(const std::string &path) {
    return JSONData::value()["contents"].has(path);
  }
  int64_t fileSize(const std::string &path) {
    return JSONData::value()["contents"][path]["size"].integer();
  }

  std::string fileMimeType(const std::string &path) {
    auto fileInfo = JSONData::value()["contents"][path];

    return fileInfo.has("Content-Type") ? fileInfo["Content-Type"].string()
                                        : std::string();
  }
  /// @todo Test
  std::string fileIdentifier(const std::string &path) {
    return JSONData::value()["contents"][path]["sha256"].string();
  }
  /// @todo Test
  std::string fileKey(const std::string &path) {
    return JSONData::value()["contents"][path]["aes256"].string();
  }

  void removeFile(const std::string &path) {
    json::Value bundle = JSONData::value();

    bundle["contents"].erase(path);
    _changeContent(bundle);
  }
  void addFile(const std::string &path, const std::string &identifier,
               const std::string &key, const std::string &mimeType = "");
  void setFileMimeType(const std::string &path, const std::string &mimeType);
  void addPreviousRevision(const std::string &identifier,
                           const std::string &key, int64_t timestamp);
  void removePreviousRevision(const std::string &identifier);
  bool hasPreviousRevision(const std::string &identifier);
  int previousRevisionCount();
  void removePreviousRevision(int index);
  std::string previousRevisionIdentifier(int index);
  std::string previousRevisionKey(int index);
  int64_t previousRevisionTimestamp(int index);
  // TODO: Make flush and reset virtual and override them here to clear cache
private:
  typedef std::map<std::string, LargeFile> LargeFiles;
  mutable LargeFiles _largeFileCache;
  void _validate();
  io::Path::StringList &_listRelative(const io::Path &path,
                                      io::Path::StringList &list);
  void _changeContent(json::Value &json);
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

  json::Value &previousList = bundle["previous"] = json::Value(json::ArrayType);

  for (auto i : previous) {
    previousList.append(json::Value() = i); // not tested
  }

  json::Value &contents = bundle["contents"] = json::Value(json::ObjectType);
  _listRelative(p, files);
  for (auto i : files) {
    io::Path absolute = p + i;
    json::Value &fileEntry = contents[std::string(i)] =
        json::Value(json::ObjectType);
    std::string mimeType = mime::fromExtension(io::Path(i).extension());
    Data fileData;

    if (absolute.size() <= data_Data_MAX_CONTENTS_SIZE) {
      fileData = SmallFile(absolute);
    } else {
      fileData = _largeFileCache[i] = LargeFile(absolute, q);
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
/// @todo Test
inline Bundle::List &Bundle::objects(Bundle::List &dataList) {
  auto parsed = JSONData::value();
  auto &contents = parsed["contents"];
  auto keys = contents.keys();

  for (auto name : keys) {
    // Consider using std::transform algorithm instead of a raw loop
    // cppcheck-suppress useStlAlgorithm
    dataList.push_back(contents[name]["sha256"].string());
  }

  for (auto entry : _largeFileCache) {
    entry.second.objects(dataList);
  }
  return dataList;
}
/// @todo Test
inline bool Bundle::update(Data &chunk) {
  auto parsed = JSONData::value();
  auto &contents = parsed["contents"];
  auto keys = contents.keys();
  auto changed = false;

  for (auto name : keys) {
    std::string identifier = contents[name]["sha256"].string();

    if (identifier == chunk.identifier()) {
      try {
        LargeFile file(chunk.data(), chunk.identifier(), chunk.key());

        _largeFileCache[name] = file;
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

  for (auto name : keys) {
    std::string identifier = contents[name]["sha256"].string();

    if (identifier == chunk.identifier()) {
      try {
        LargeFile file(chunk.data(), chunk.identifier(), chunk.key());

        _largeFileCache[name] = file;
        changed = true;
      } catch (const msg::Exception &exception) {
        io::Path filePath = path + name;
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

  for (auto entry : _largeFileCache) {
    changed = entry.second.write(path + entry.first, chunk) | changed;
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
    return false; // not tested
  }

  for (auto name : existingFiles) {
    if (!contents.has(name)) {
      return false; // not tested
    }
  }

  json::Value::StringList keys = contents.keys();

  for (auto name : keys) {
    if (std::find(existingFiles.begin(), existingFiles.end(), name) ==
        existingFiles.end()) {
      return false; // not tested
    }
  }

  for (auto entry : _largeFileCache) {
    // Consider using std::any_of algorithm instead of a raw loop.
    // cppcheck-suppress useStlAlgorithm
    if (entry.second != other + entry.first) {
      return false; // not tested
    }
  }

  for (auto name : keys) {
    if (_largeFileCache.find(name) == _largeFileCache.end()) {
      try {
        if (SmallFile(other + name).key() !=
            contents[name]["aes256"].string()) {
          return false; // not tested
        }
      } catch (const msg::Exception &) {
        return false;
      }
    }
  }

  return true;
}

inline void Bundle::resetComment(const std::string &comment) {
  json::Value bundle = JSONData::value();

  if (comment.size() == 0) {
    bundle.erase("comments");
  } else {
    bundle["comments"] = comment;
  }
  _changeContent(bundle);
}
inline Bundle::List &Bundle::files(Bundle::List &fileList) {
  json::Value parsed = JSONData::value();
  const json::Value &contents = parsed["contents"];
  auto keys = contents.keys();

  std::copy(keys.begin(), keys.end(), std::back_inserter(fileList));

  return fileList;
}
/// @todo Test
inline void Bundle::addFile(const std::string &path,
                            const std::string &identifier,
                            const std::string &key,
                            const std::string &mimeType) {
  json::Value bundle = JSONData::value();
  json::Value info(json::ObjectType);
  std::string useMimeType =
      mimeType.size() == 0 ? mime::fromExtension(io::Path(path).extension())
                           : mimeType;

  info["sha256"] = identifier;
  info["aes256"] = key;
  if (mimeType.size() > 0) {
    info["Content-Type"] = useMimeType;
  }
  bundle["contents"][path] = info;
  _changeContent(bundle);
}

inline void Bundle::setFileMimeType(const std::string &path,
                                    const std::string &mimeType) {
  json::Value bundle = JSONData::value();

  if (mimeType.size() == 0) {
    bundle["contents"][path].erase("Content-Type");
  } else {
    bundle["contents"][path]["Content-Type"] = mimeType;
  }
  _changeContent(bundle);
}

inline void Bundle::addPreviousRevision(const std::string &identifier,
                                        const std::string &key,
                                        int64_t timestamp) {
  json::Value bundle = JSONData::value();
  json::Value &previous = bundle["previous"];
  json::Value info(json::ObjectType);
  const auto max = previous.count();
  int insertHere = 0;

  info["sha256"] = identifier;
  info["aes256"] = key;
  info["timestamp"] = timestamp;

  while ((insertHere < max) && (previous[insertHere]["timestamp"].integer() >
                                timestamp)) { // not tested
    ++insertHere;                             // not tested
  }
  previous.insert(info, insertHere);
  _changeContent(bundle);
}
/// @todo Test
inline void Bundle::removePreviousRevision(const std::string &identifier) {
  json::Value bundle = JSONData::value();
  json::Value &previous = bundle["previous"];
  const auto max = previous.count();

  for (auto i = 0; i < max; ++i) {
    if (previous[i]["sha256"].string() == identifier) {
      previous.erase(i, i + 1);
      break;
    }
  }

  _changeContent(bundle);
}

inline bool Bundle::hasPreviousRevision(const std::string &identifier) {
  json::Value bundle = JSONData::value();
  json::Value &previous = bundle["previous"];
  const auto max = previous.count();

  for (auto i = 0; i < max; ++i) {
    if (previous[i]["sha256"].string() == identifier) {
      return true;
    }
  }
  return false; // not tested
}

inline int Bundle::previousRevisionCount() {
  json::Value bundle = JSONData::value();
  return bundle["previous"].count();
}
/// @todo Test
inline void Bundle::removePreviousRevision(int index) {
  json::Value bundle = JSONData::value();
  bundle["previous"].erase(index, index + 1);
  _changeContent(bundle);
}

inline std::string Bundle::previousRevisionIdentifier(int index) {
  json::Value bundle = JSONData::value();

  return bundle["previous"][index]["sha256"].string();
}

inline std::string Bundle::previousRevisionKey(int index) {
  json::Value bundle = JSONData::value();

  return bundle["previous"][index]["aes256"].string();
}

inline int64_t Bundle::previousRevisionTimestamp(int index) {
  json::Value bundle = JSONData::value();

  return bundle["previous"][index]["timestamp"].integer();
}

inline void Bundle::_validate() {
  json::Value parsed = JSONData::value();

  AssertMessageException(parsed.is(json::ObjectType));
  AssertMessageException(parsed.count() >= 1);
  json::Value &contents =
      JSONData::_validateKey(parsed, "contents", json::ObjectType);
  json::Value::StringList keys = contents.keys();

  JSONData::_validatePositiveInteger(parsed, "timestamp");
  JSONData::_validateOptionalKey(parsed, "comments", json::StringType);
  for (auto name : keys) {
    json::Value &entry = contents[name];

    JSONData::_validateHash(entry, "sha256");
    JSONData::_validateHash(entry, "aes256");
    JSONData::_validatePositiveInteger(entry, "size");
    JSONData::_validateOptionalKey(parsed, "Content-Type", json::StringType);
  }

  if (parsed.has("previous")) {
    json::Value &previous =
        JSONData::_validateKey(parsed, "previous", json::ArrayType);
    const int count = previous.count();

    for (int i = 0; i < count; ++i) {
      json::Value &entry = previous[i];
      // TODO Validate the timestamps are sequential
      JSONData::_validatePositiveInteger(entry, "timestamp");
      JSONData::_validateHash(entry, "sha256");
      JSONData::_validateHash(entry, "aes256");
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
    names.clear();
    directory.list(io::Path::NameOnly, names, io::Path::FlatListing);
    for (auto name : names) {
      io::Path relative = relativeBase + name;
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

inline void Bundle::_changeContent(json::Value &json) {
  json["timestamp"] =
      int64_t(dt::DateTime().seconds() + AddToConvertToAppleCocoaEpoch);
  JSONData::assign(json, Data::Encrypted);
  _validate();
}

} // namespace data

#endif // __Bundle_h__
