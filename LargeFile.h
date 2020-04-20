#ifndef __LargeFile_h__
#define __LargeFile_h__

#include "libernet/Data.h"
#include "libernet/JSONData.h"
#include "libernet/SmallFile.h"
#include "os/Exception.h"
#include "os/File.h"
#include "os/Hash.h"
#include "os/Path.h"
#include "os/Queue.h"
#include <algorithm>
#include <vector>

namespace data {

class LargeFile : public JSONData {
public:
  typedef exec::Queue<Data> Queue;
  typedef std::vector<std::string> List;
  LargeFile() : JSONData() {}
  LargeFile(const io::Path &p, Queue &q) : JSONData() { assign(p, q); }
  LargeFile(const std::string &data, const std::string &identifier,
            const std::string &key);
  LargeFile(const LargeFile &other) : JSONData(other) {}
  virtual ~LargeFile() {}
  LargeFile &assign(const io::Path &path, Queue &queue);
  LargeFile &operator=(const LargeFile &other) {
    JSONData::operator=(other);
    return *this;
  }
  LargeFile &assign(const std::string &data, const std::string &identifier,
                    const std::string &key);
  bool operator==(const Data &other) const { return Data::operator==(other); }
  bool operator!=(const Data &other) const { return !(*this == other); }
  List &objects(List &dataList);
  bool write(const io::Path &path, const Data &chunk);
  bool operator==(const io::Path &other);
  bool operator!=(const io::Path &other) { return !(*this == other); }

private:
  static int64_t _validatePositiveInteger(json::Value &value,
                                          const std::string &name);
  static void _validateHash(json::Value &value, const std::string &name);
  json::Value _validate();
};

inline Data loadFile(const io::Path &path, LargeFile::Queue &queue) {
  if (path.size() <= data_Data_MAX_CONTENTS_SIZE) {
    return SmallFile(path);
  }
  return LargeFile(path, queue);
}

inline LargeFile::LargeFile(const std::string &data,
                            const std::string &identifier,
                            const std::string &key)
    : JSONData(data, identifier, key) {
  _validate();
}

inline LargeFile &LargeFile::assign(const io::Path &path,
                                    LargeFile::Queue &queue) {
  int64_t dataLeft = static_cast<int64_t>(path.size());
  json::Value parts(json::ArrayType);
  json::Value entry(json::ObjectType);
  io::File file(path, io::File::Binary, io::File::ReadOnly);
  std::string buffer;
  Data block;

  entry["size"] = dataLeft;
  parts.append(entry);

  while (dataLeft > 0) {
    size_t readAmount =
        std::min(dataLeft, static_cast<int64_t>(data_Data_MAX_CONTENTS_SIZE));

    file.read(buffer, readAmount);
    dataLeft -= buffer.size();
    block.assign(buffer, Data::Encrypted);

    entry["sha256"] = block.identifier();
    entry["aes256"] = block.key();
    entry["size"] = static_cast<int64_t>(buffer.size());
    parts.append(entry);

    queue.enqueue(Data(block.data(), block.identifier()));
  }
  JSONData::assign(parts, Data::Encrypted);
  return *this;
}

inline LargeFile &LargeFile::assign(const std::string &data,
                                    const std::string &identifier,
                                    const std::string &key) {
  Data::assign(data, identifier, key);
  _validate();
  return *this;
}

inline LargeFile::List &LargeFile::objects(LargeFile::List &dataList) {
  json::Value parsed = JSONData::value();
  const int count = parsed.count();

  dataList.clear();
  for (int index = 1; index < count; ++index) {
    dataList.push_back(parsed[index]["sha256"].string());
  }

  return dataList;
}

inline bool LargeFile::write(const io::Path &path, const Data &chunk) {
  json::Value parsed = JSONData::value();
  const int count = parsed.count();
  int64_t offset = 0;
  int index;

  for (index = 1; (index < count) &&
                  (chunk.identifier() != parsed[index]["sha256"].string());
       ++index) {
    offset += parsed[index]["size"].integer();
  }

  if (index >= count) {
    return false; // no test coverage
  }

  std::string contents =
      Data(chunk.data(), chunk.identifier(), parsed[index]["aes256"].string())
          .contents();
  io::File file(path, io::File::Binary, io::File::ReadWrite);

  file.write(contents, static_cast<off_t>(offset), io::File::FromStart);

  return true;
}

inline bool LargeFile::operator==(const io::Path &other) {
  json::Value parsed = JSONData::value();
  const int count = parsed.count();

  if (other.size() != static_cast<off_t>(parsed[0]["size"].integer())) {
    return false; // no test coverage
  }

  io::File file(other, io::File::Binary, io::File::ReadOnly);
  std::string buffer;
  hash::sha256 blockHash;

  for (int index = 1; index < count; ++index) {
    int64_t blockSize = parsed[index]["size"].integer();

    file.read(buffer, static_cast<size_t>(blockSize));
    blockHash.reset(buffer);

    if (blockHash.hex() != parsed[index]["aes256"].string()) {
      return false; // no test coverage
    }
  }
  return true;
}

inline int64_t LargeFile::_validatePositiveInteger(json::Value &value,
                                                   const std::string &name) {
  AssertMessageException(value.has(name));
  AssertMessageException(value[name].is(json::IntegerType));
  AssertMessageException(value[name].integer() > 0);
  return value[name].integer();
}

inline void LargeFile::_validateHash(json::Value &value,
                                     const std::string &name) {
  AssertMessageException(value.has(name));
  AssertMessageException(value[name].is(json::StringType));
  hash::sha256().reset(value[name].string().c_str());
}

inline json::Value LargeFile::_validate() {
  int count, index;
  int64_t totalSize, sumOfBlocks = 0;
  json::Value parsed = JSONData::value();

  AssertMessageException(parsed.is(json::ArrayType));
  AssertMessageException((count = parsed.count()) >= 2);

  AssertMessageException(
      (totalSize = _validatePositiveInteger(parsed[0], "size")) >=
      data_Data_MAX_CONTENTS_SIZE);

  for (index = 1; index < count; ++index) {
    int64_t blockSize = _validatePositiveInteger(parsed[index], "size");

    AssertMessageException((blockSize) <= data_Data_MAX_CONTENTS_SIZE);
    AssertMessageException((sumOfBlocks += blockSize) <= totalSize);

    _validateHash(parsed[index], "sha256");
    _validateHash(parsed[index], "aes256");
  }

  AssertMessageException(sumOfBlocks == totalSize);
  return parsed;
}

} // namespace data

#endif // __LargeFile_h__
