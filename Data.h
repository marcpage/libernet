#ifndef __Data_h__
#define __Data_h__

#include "os/Exception.h"
#include "os/Hash.h"
#include "os/SymmetricEncrypt.h"
#include "os/ZCompression.h"
#include <ctype.h>
#include <string>

namespace data {

class Exception : public msg::Exception {
public:
  // cppcheck-suppress noExplicitConstructor
  Exception(const std::string &message, const char *file = nullptr,
            int line = 0)
      : msg::Exception(message, file, line) {}
  // cppcheck-suppress missingOverride
  virtual ~Exception() {}
};

class CorruptData : public Exception {
public:
  // cppcheck-suppress noExplicitConstructor
  CorruptData(const std::string &message, const char *file = nullptr,
              int line = 0)
      : Exception(message, file, line) {}
  virtual ~CorruptData() {}
};

class InvalidIdentifier : public Exception {
public:
  // cppcheck-suppress noExplicitConstructor
  InvalidIdentifier(const std::string &message, const char *file = nullptr,
                    int line = 0)
      : Exception(message, file, line) {}
  virtual ~InvalidIdentifier() {}
};

class DataTooBig : public Exception {
public:
  // cppcheck-suppress noExplicitConstructor
  DataTooBig(const std::string &message, const char *file = nullptr,
             int line = 0)
      : Exception(message, file, line) {}
  virtual ~DataTooBig() {}
};

class Data {
public:
  enum Encryption { Encrypted, Unencrypted };
  enum Compression { Decompress, NoCompression };
  Data() : _contents(), _data(), _key(), _identifier() {}
  Data(const Data &other);
  Data(const std::string &contents, Encryption encryption);
  Data(const std::string &data, const std::string &identifier,
       const std::string &key);
  Data(const std::string &data, const std::string &identifier);
  Data &assign(const std::string &contents, Encryption encryption);
  Data &assign(const std::string &data, const std::string &identifier,
               const std::string &key);
  Data &assign(const std::string &data, const std::string &identifier);
  std::string contents(Compression compression = NoCompression) {
    std::string buffer;
    return contents(buffer, compression);
  }
  std::string data() const {
    std::string buffer;
    return data(buffer);
  }
  std::string key() {
    std::string buffer;
    return key(buffer);
  }
  std::string identifier() const {
    std::string buffer;
    return identifier(buffer);
  }
  std::string &contents(std::string &buffer,
                        Compression compression = NoCompression);
  std::string &data(std::string &buffer) const;
  std::string &key(std::string &buffer);
  std::string &identifier(std::string &buffer) const;
  Data &operator=(const Data &other);
  bool operator==(const Data &other) const;
  bool operator!=(const Data &other) const { return !(*this == other); }
  bool encrypted() const;
  Data &flush();
  Data &reset();

private:
  /** Raw, original contents.
          If not encrypted, may be empty.
  */
  std::string _contents;
  /** The data being shipped around.
          If _key is empty, may be compressed contents.
  */
  mutable std::string _data;
  /** The encryption key.
          Should be empty if not encrypted. Hash of _contents.
  */
  std::string _key;
  mutable std::string _identifier; ///< The hash of _data

  void _validateIdentifier() const;
  void _validateSize();
  void _calculateContents();
  void _calculateData() const;
  void _calculateKey() { _key = hash::sha256(_contents).hex(); }

  static bool _hexEquals(const std::string &h1, const std::string &h2);
};

inline Data::Data(const Data &other)
    : _contents(other._contents), _data(other._data), _key(other._key),
      _identifier(other._identifier) {}

inline Data::Data(const std::string &contents, Encryption encryption)
    : _contents(contents), _data(), _key(), _identifier() {
  _validateSize();
  if (Encrypted == encryption) {
    _calculateKey();
  }
}

inline Data::Data(const std::string &data, const std::string &identifier,
                  const std::string &key)
    : _contents(), _data(data), _key(key), _identifier(identifier) {
  _validateSize();
  _validateIdentifier();
}

inline Data::Data(const std::string &data, const std::string &identifier)
    : _contents(), _data(data), _key(), _identifier(identifier) {
  _validateSize();
  _validateIdentifier();
}

inline Data &Data::assign(const std::string &contents, Encryption encryption) {
  _contents = contents;
  _validateSize();
  if (Encrypted == encryption) {
    _calculateKey();
  } else {
    _key.clear();
  }
  _data.clear();
  _identifier.clear();
  return *this;
}

inline Data &Data::assign(const std::string &data,
                          const std::string &identifier,
                          const std::string &key) {
  _data = data;
  _validateSize();
  _identifier = identifier;
  _validateIdentifier();
  _contents.clear();
  _key = key;
  return *this;
}

inline Data &Data::assign(const std::string &data,
                          const std::string &identifier) {
  _data = data;
  _validateSize();
  _identifier = identifier;
  _validateIdentifier();
  _contents.clear();
  _key.clear();
  return *this;
}

/** Gets the contents of the data block.
        Encrypted blocks can detect compression since the key is the hash of the
   contents. Unencrypted blocks cannot detect if the data is compressed, hence
   the compression field. For JSON contents, you will probably pass Decompress
   for compression.
        @param buffer The buffer to fill with the contents.
        @param compression If data is not encrypted and Decompress then
   decompressed contents will be returned
        @return buffer
*/
inline std::string &Data::contents(std::string &buffer,
                                   Compression compression) {
  if (_contents.size() == 0) {
    if (!encrypted()) {
      if (NoCompression == compression) {
        return buffer = _data;
      }
      return z::uncompress(_data, buffer, 1024 * 1024);
    }
    _calculateContents();
  }
  return buffer = _contents;
}

inline std::string &Data::data(std::string &buffer) const {
  if (_data.size() == 0) {
    _calculateData();
  }
  return buffer = _data;
}

inline std::string &Data::key(std::string &buffer) { return buffer = _key; }

inline std::string &Data::identifier(std::string &buffer) const {
  if (_identifier.size() == 0) {
    _calculateData();
  }
  return buffer = _identifier;
}

inline Data &Data::operator=(const Data &other) {
  if (this != &other) {
    _contents = other._contents;
    _data = other._data;
    _key = other._key;
    _identifier = other._identifier;
  }
  return *this;
}

inline bool Data::operator==(const Data &other) const {
  if (encrypted() && other.encrypted()) {
    return _hexEquals(_key, other._key);
  }
  if ((_contents.size() > 0) && (other._contents.size() > 0)) {
    return _contents == other._contents;
  }
  if ((_identifier.size() > 0) && (other._identifier.size() > 0)) {
    return _hexEquals(_identifier, other._identifier);
  }
  if (_data.size() == 0) {
    _calculateData();
  } else if (other._data.size() == 0) {
    other._calculateData();
  }
  AssertMessageException((_identifier.size() > 0) &&
                         (other._identifier.size() > 0));
  return (*this == other);
}

inline bool Data::encrypted() const { return _key.size() > 0; }

inline Data &Data::flush() {
  if (_data.size() > 0) {
    _contents.clear();
    _contents.reserve();
  }
  return *this;
}

inline Data &Data::reset() {
  if (_contents.size() > 0) {
    _data.clear();
    _data.reserve();
    _key.clear();
    _key.reserve();
    _identifier.clear();
    _identifier.reserve();
  }
  return *this;
}

inline void Data::_validateSize() {
  if (_contents.size() > 1024 * 1024) {
    throw DataTooBig("Content size (" + std::to_string(_contents.size()) +
                         ") is larger than the maximum (" +
                         std::to_string(1024 * 1024) + ").",
                     __FILE__, __LINE__);
  }
  if (_data.size() > 1024 * 1024) {
    throw DataTooBig("Content size (" + std::to_string(_contents.size()) +
                         ") is larger than the maximum (" +
                         std::to_string(1024 * 1024) + ").",
                     __FILE__, __LINE__);
  }
}

inline void Data::_validateIdentifier() const {
  const std::string hash = hash::sha256(_data).hex();

  if (_data.size() == 0) {
    _calculateData(); // not covered in testing, cannot be reached
  }
  if (!_hexEquals(hash, _identifier)) {
    throw InvalidIdentifier("Identifier (" + _identifier +
                                ") does not match hash (" + _identifier + ").",
                            __FILE__, __LINE__);
  }
}

inline void Data::_calculateContents() {
  if (encrypted()) {
    hash::sha256 keyData = hash::sha256::fromHex(_key);
    crypto::AES256 key(reinterpret_cast<const char *>(keyData.buffer()),
                       keyData.size());

    key.crypto::SymmetricKey::decryptInPlace(_data, "", _contents);

    const std::string decryptedHash = hash::sha256(_contents).hex();

    if (!_hexEquals(decryptedHash, _key)) {
      std::string buffer;

      z::uncompress(_contents, buffer, 1024 * 1024);

      const std::string uncompressedHash = hash::sha256(buffer).hex();

      if (!_hexEquals(uncompressedHash, _key)) {
        throw CorruptData("Key (" + _key + ") does not match decrypted hash (" +
                              decryptedHash + ") nor uncompressed hash (" +
                              uncompressedHash + ")",
                          __FILE__, __LINE__);
      }
      _contents = buffer;
    }
  }
}

inline void Data::_calculateData() const {
  std::string compressed;
  const std::string *data;

  z::compress(_contents, compressed, 9);
  data = compressed.size() < _contents.size() ? &compressed : &_contents;
  if (encrypted()) {
    hash::sha256 keyData = hash::sha256::fromHex(_key);
    crypto::AES256 key(reinterpret_cast<const char *>(keyData.buffer()),
                       keyData.size());

    key.crypto::SymmetricKey::encryptInPlace(*data, "", _data);
  } else {
    _data = *data;
  }
  _identifier = hash::sha256(_data).hex();
}

inline bool Data::_hexEquals(const std::string &h1, const std::string &h2) {
  if (h1.size() != h2.size()) {
    return false; // not covered in testing
  }
  std::string::const_iterator c1 = h1.begin();
  std::string::const_iterator c2 = h2.begin();

  while ((tolower(*c1) == tolower(*c2)) && (c1 != h1.end())) {
    ++c1;
    ++c2;
  }
  return (c1 == h1.end());
}

} // namespace data

#endif // __Data_h__
