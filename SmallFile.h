#ifndef __SmallFile_h__
#define __SmallFile_h__

#include "libernet/Data.h"
#include "os/Path.h"

namespace data {

class SmallFile : public Data {
public:
  SmallFile() : Data() {}
  virtual ~SmallFile() {}
  explicit SmallFile(const io::Path &path) : Data() { _loadFromFile(path); }
  explicit SmallFile(const std::string &data) : Data(data, Data::Encrypted) {}
  SmallFile(const std::string &data, const std::string &identifier,
            const std::string &key);
  SmallFile &assign(const io::Path &path) {
    _loadFromFile(path);
    return *this;
  }
  SmallFile &assign(const std::string &data) {
    Data::assign(data, Data::Encrypted);
    return *this;
  }
  SmallFile &assign(const std::string &data, const std::string &identifier,
                    const std::string &key);
  SmallFile &operator=(const Data &data);
  bool operator==(const Data &other) const { return Data::operator==(other); }
  bool operator!=(const Data &other) const { return !(*this == other); }
  SmallFile &write(const io::Path &location);
  bool operator==(const io::Path &other);
  bool operator!=(const io::Path &other) { return !(*this == other); }

private:
  void _loadFromFile(const io::Path &path);
};

inline SmallFile::SmallFile(const std::string &data,
                            const std::string &identifier,
                            const std::string &key)
    : Data(data, identifier, key) {}

inline SmallFile &SmallFile::assign(const std::string &data,
                                    const std::string &identifier,
                                    const std::string &key) {
  Data::assign(data, identifier, key);
  return *this;
}

inline SmallFile &SmallFile::operator=(const Data &data) {
  Data::operator=(data);
  return *this;
}

inline SmallFile &SmallFile::write(const io::Path &location) {
  location.write(Data::contents(), io::File::Binary);
  return *this;
}

bool SmallFile::operator==(const io::Path &other) {
  auto myContents = contents();
  auto fileSize = other.size();

  if (static_cast<decltype(fileSize)>(myContents.size()) != fileSize) {
    return false; // not tested
  }

  return SmallFile(other).contents() == myContents;
}

inline void SmallFile::_loadFromFile(const io::Path &path) {
  std::string buffer;
  const off_t size = path.size();

  if (size > data_Data_MAX_CONTENTS_SIZE) {
    throw DataTooBig("File size (" + std::to_string(size) +
                         ") is larger than the maximum (" +
                         std::to_string(data_Data_MAX_CONTENTS_SIZE) +
                         "): " + std::string(path),
                     __FILE__, __LINE__);
  }
  path.contents(buffer, io::File::Binary, 0, size);
  Data::assign(buffer, Data::Encrypted);
}

} // namespace data

#endif // __SmallFile_h__
