#ifndef __LargeFile_h__
#define __LargeFile_h__

#include "libernet/Data.h"
#include "libernet/JSONData.h"
#include "os/Exception.h"
#include "os/File.h"
#include "os/Hash.h"
#include "os/Path.h"
#include "os/Queue.h"
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
  bool write(const io::Path &file, const Data &chunk);
  bool operator==(const io::Path &other) const;

private:
  static int64_t _validatePositiveInteger(const json::Value &value,
                                          const std::string &name);
  static void _validateHash(const json::Value &value, const std::string &name);
  json::Value _validate();
};

inline LargeFile::LargeFile(const std::string &data,
                            const std::string &identifier,
                            const std::string &key)
    : JSONData(data, identifier, key) {
  _validate();
}

inline LargeFile &LargeFile::assign(const io::Path &path, Queue &queue) {
	int64_t dataLeft = static_cast<int64_t>(path.size());
	json::Value parts(json::ArrayType);
	json::Value entry(json::ObjectType);
  	io::File file(file, io::File::Binary, io::File::ReadOnly);
  	std::string buffer;

	entry["size"] = dataLeft;
	parts.append(entry);

	while (dataLeft > 0) {
		size_t readAmount = std::min(dataLeft, 1024 * 1024);

		file.read(buffer, readAmount);
	}
	// TODO: implement after updating a hash is supported
}

inline LargeFile &LargeFile::assign(const std::string &data,
                                    const std::string &identifier,
                                    const std::string &key) {
  Data::assign(data, identifier, key);
  _validate();
}

inline List &LargeFile::objects(List &dataList) {
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

  for (index = 1;
       (index < count) && (chunk.identifier() == parsed[index]["sha256"]);
       ++index) {
    offset += parsed[index]["size"].integer();
  }

  if (index >= count) {
    return false;
  }

  std::string contents =
      data::Data(chunk.data(), chunk.identifier(), parsed[index]["aes256"])
          .contents();
  io::File file(path, io::File::Binary, io::File::ReadWrite);

  file.write(contents, static_cast<off_t>(offset), io::File::FromStart);

  return true;
}

inline bool LargeFile::operator==(const io::Path &other) const {
  json::Value parsed = JSONData::value();
  const int count = parsed.count();

  if (other.size() != static_cast<off_t>(parsed[0]["size"].integer())) {
    return false;
  }

  io::File file(file, io::File::Binary, io::File::ReadOnly);
  std::string buffer;
  hash::sha256 blockHash;

  for (int index = 1; index < count; ++index) {
    int64_t blockSize = parsed[index]["size"].integer();

    file.read(buffer, static_cast<size_t>(blockSize));
    blockHash.reset(buffer);

    if (blockHash.hex() != parsed[index]["sha256"].string()) {
    	return false;
    }

  }
}

inline int64_t _validatePositiveInteger(const json::Value &value,
                                        const std::string &name) {
  AssertMessageException(value.has(name));
  AssertMessageException(value[name].is(json::IntegerType));
  AssertMessageException(value[name].integer() > 0);
  return value[name].integer();
}

inline void _validateHash(const json::Value &value, const std::string &name) {
  AssertMessageException(value.has(name));
  AssertMessageException(value[name].is(json::StringType));
  hash::sha256().reset(value[name].string().c_str());
}

inline json::Value LargeFile::_validate() {
  int count, index;
  int64_t totalSize, sumOfBlocks = 0;
  const int64_t OneMegaByte = 1024 * 1024;
  json::Value parsed = JSONData::value();

  AssertMessageException(parsed.is(json::ArrayType));
  AssertMessageException((count = parsed.count()) >= 2);

  AssertMessageException(
      (totalSize = _validatePositiveInteger(parsed[0], "size")) >= OneMegaByte);

  for (index = 1; index < count; ++index) {
    int64_t blockSize = _validatePositiveInteger(parsed[index], "size");

    AssertMessageException((blockSize) <= OneMegaByte);
    AssertMessageException((sumOfBlocks += blockSize) <= totalSize);
    _validateHash(parsed[index], "sha256");
    _validateHash(parsed[index], "aes256");
  }

  AssertMessageException(sumOfBlocks == totalSize);
  return parsed;
}

} // namespace data

#endif // __LargeFile_h__
