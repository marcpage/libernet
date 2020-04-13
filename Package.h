#ifndef __Package_h__
#define __Package_h__

#include "os/Hash.h"
#include "os/MemoryMappedFile.h"
#include "os/Path.h"
#include "os/Queue.h"
#include "os/SymmetricEncrypt.h"
#include "os/ZCompression.h"
#include "protocol/JSON.h"
#include <string>
#include <thread>

namespace pkg {

/**
        @todo Add an assertHex(const std::string&)
*/
inline void assertIdentifier(const std::string &nameType,
                             const std::string & /*name*/,
                             const std::string &keyType,
                             const std::string &key) {
  if (nameType != "sha256") {
    throw json::WrongType("Expected data identifier to be sha256, but found '" +
                              nameType + "'",
                          __FILE__, __LINE__);
  } else if ((keyType.size() > 0) && (keyType != "aes256/sha256")) {
    throw json::WrongType(
        "Expected key type to be aes256/sha256 or blank, but found '" +
            keyType + "'",
        __FILE__, __LINE__);
  } else if ((keyType.size() == 0) && (key.size() != 0)) {
    throw json::WrongType(
        "Expected empty key if keyType is empty, but found key: '" + key + "'",
        __FILE__, __LINE__);
  } else if ((keyType.size() != 0) && (key.size() == 0)) {
    throw json::WrongType("We have an empty key, but a key type of: '" +
                              keyType + "'",
                          __FILE__, __LINE__);
  }
}

/** Takes a resource identifier and breaks it down into its parts.
        @param identifier The identifier of the form
   [HASH_TYPE]:[HASH]:[KEY_TYPE]:[KEY] or [HASH_TYPE]:[HASH]
        @param nameType [OUTPUT] The type of data identifier (HASH_TYPE). May
   return empty if invalid form
        @param name [OUTPUT] The name of the data (HASH). May return empty if
   invalid form
        @param keyType [OUTPUT] The type of key to decrypt the data (KEY_TYPE).
                                        May return empty if no encryption was
   specified or invalid form
        @param key [OUTPUT] The encryption key to decrypt the data (KEY).
                                        May return empty if no encryption was
   specified or invalid form
*/
inline void splitIdentifier(const std::string &identifier,
                            std::string &nameType, std::string &name,
                            std::string &keyType, std::string &key) {
  const std::string::size_type firstColon = identifier.find(':');
  const std::string::size_type secondColon =
      identifier.find(':', firstColon + 1);
  const std::string::size_type thirdColon =
      identifier.find(':', secondColon + 1);

  nameType.clear();
  name.clear();
  keyType.clear();
  key.clear();
  if (std::string::npos != firstColon) {
    nameType.assign(identifier, 0, firstColon);
    if (std::string::npos != secondColon) {
      name.assign(identifier, firstColon + 1, secondColon - firstColon - 1);
      if (std::string::npos != thirdColon) {
        keyType.assign(identifier, secondColon + 1,
                       thirdColon - secondColon - 1);
        key.assign(identifier, thirdColon + 1);
      }
    } else {
      name.assign(identifier, firstColon + 1);
    }
  }
  assertIdentifier(nameType, name, keyType, key);
}

/** Optionally compresses and encrypts data, returning identifiers for the data.
        @param sourceData The actual data to compress, encrypt, and store
        @param packedData [OUTPUT] The (possibly) compressed and encrypted data
        @param dataName [OUTPUT] The name used to reference the data
        @return The full identifier of the data, used to request and decrypt the
   data
*/
inline std::string packData(const std::string &sourceData,
                            std::string &packedData, std::string &dataName) {
  hash::sha256 sourceHash(sourceData);
  std::string compressedData = z::compress(sourceData, 9);
  const std::string &dataToEncrypt =
      (sourceData.size() > compressedData.size()) ? compressedData : sourceData;
  std::string key(reinterpret_cast<const char *>(sourceHash.buffer()),
                  sourceHash.size());

  packedData = crypto::AES256(key).encrypt(dataToEncrypt);
  dataName = "sha256:" + hash::sha256(packedData).hex();
  return dataName + ":" + "aes256/sha256:" + sourceHash.hex();
}

inline std::string packData(const std::string &sourceData,
                            std::string &packedData) {
  std::string dataName;

  return packData(sourceData, packedData, dataName);
}

/** Given a block of data, pack it, store it, and return its full identifier to
   get the data back. Data will not be written if it is already in the
   storagePath.
        @param sourceData The data to store
        @param storagePath The path to store the packed data
        @return The full identifier of the data
*/
inline std::string storeData(const std::string &sourceData,
                             const io::Path &storagePath) {
  std::string packedData, dataName;
  std::string identifier = packData(sourceData, packedData, dataName);
  const io::Path dataPath(storagePath + dataName);

  if (!dataPath.exists()) {
    dataPath.write(packedData);
  }
  return identifier;
}

/** Pack a portion of a file, store it, and return its identifier.
        @param source The path to the file
        @param offset The offset in the file start getting data to store
        @param size The number of bytes to store from offset
        @param storagePath The path to the directory to store the data
        @return The identifier for the portion of the file that was stored
*/
inline std::string encryptFilePart(const io::Path &source, const off_t offset,
                                   const size_t size,
                                   const io::Path &storagePath) {
  return storeData(source.contents(io::File::Binary, offset, size),
                   storagePath);
}

/** Encrypt and store the contents of a file and return the file identifier.
        Files in broken up into 1 MiB chunks, each separately compressed
   (optionally), encrypted, and stored. A resulting identifier that can
   identifer and restore the contents of the file is returned.
        @param source The path to the file to encrypt and store
        @param storagePath The path to the directory to store data parts
        @return The identifier for the entire file
*/
inline std::string encryptFile(const io::Path &source,
                               const io::Path &storagePath) {
  const off_t fileSize = source.size();
  const size_t filePartSize = 1024 * 1024; // 1 MiB max part size

  // No need to compress or encrypt anything, its just an empty file
  if (0 == fileSize) {
    // e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    static const std::string emptyHash = hash::sha256("", 0).hex();

    return "sha256:" + emptyHash;
  }

  const int parts = (fileSize - 1) / filePartSize + 1;
  const size_t lastPartSize = fileSize - (parts - 1) * filePartSize;
  std::string identifier;
  std::string prefix;

  for (int i = 0; i < parts; ++i) {
    // Each part will be filePartSize, but the remaining extra will be smaller
    const size_t partSize = (i == parts - 1) ? lastPartSize : filePartSize;
    const std::string partIdentifier =
        encryptFilePart(source, i * filePartSize, partSize, storagePath);

    identifier = identifier + prefix + partIdentifier;
    prefix = ",";
  }

  return identifier;
}

/** Given the path to a directory, compresses, encrypts, and stores the data
   parts.
        @param path The directory to store
        @param storagePath The path to a directory to store data parts in
        @return The json data that describes the directory so it can be restored
*/
inline json::Value directoryInfo(const io::Path &path,
                                 const io::Path &storagePath) {
  json::Value listing = json::Value(json::ObjectType);
  const io::Path storagePathAbsolute = storagePath.absolute();
  const io::Path listingPathAbsolute = path.absolute();

  io::Path::StringList contents = listingPathAbsolute.list(
      io::Path::PathAndName, io::Path::RecursiveListing);

  for (auto entry = contents.begin(); entry != contents.end(); ++entry) {
    if (!io::Path::endsWithPathSeparator(*entry)) {
      listing["files"][io::Path(*entry).relativeTo(listingPathAbsolute)]
             ["contents"] = encryptFile(*entry, storagePathAbsolute);
      listing["files"][io::Path(*entry).relativeTo(listingPathAbsolute)]
             ["size"] = io::Path(*entry).size();
    } else {
      json::Value directoryName;
      directoryName =
          std::string(io::Path(*entry).relativeTo(listingPathAbsolute));
      listing["directories"].append(directoryName);
    }
  }

  return listing;
}

/** Package a directory and store all data necessary to retrieve and restore the
   directory.
        @param path The directory to store
        @param storagePath The path to a directory to store data parts in
        @return The identifier from which the directory can be restored
*/
inline std::string packageDirectory(const io::Path &path,
                                    const io::Path &storagePath) {
  return storeData(directoryInfo(path, storagePath), storagePath.absolute());
}

inline std::string unpackData(const std::string &packedData,
                              const std::string &identifier) {
  std::string nameType, name, keyType, key;
  std::string compressedData;
  std::string uncompressedData;
  std::string finalDataHash;

  splitIdentifier(identifier, nameType, name, keyType, key);

  if (key.size() > 0) {
    hash::sha256 sourceHash = hash::sha256::fromHex(key);
    std::string keyData(reinterpret_cast<const char *>(sourceHash.buffer()),
                        sourceHash.size());
    std::string unpackedData = crypto::AES256(keyData).decrypt(packedData);
    hash::sha256 packagedHash(packedData);

    if (name != packagedHash.hex()) {
      throw json::WrongType("Name does not match contents: name='" + name +
                                "' hash='" + packagedHash.hex() + "'",
                            __FILE__, __LINE__);
    }

    if (hash::sha256(unpackedData).hex() == key) {
      return unpackedData;
    }

    compressedData = unpackedData;
    finalDataHash = key;
  } else if (hash::sha256(packedData).hex() == name) {
    return packedData;
  } else {
    compressedData = packedData;
    finalDataHash = name;
  }

  z::uncompress(compressedData, uncompressedData, 1024 * 1024);

  if (hash::sha256(uncompressedData).hex() != finalDataHash) {
    throw json::WrongType("Name does not match contents: expected='" +
                              finalDataHash + "' hash='" +
                              hash::sha256(uncompressedData).hex() + "'",
                          __FILE__, __LINE__);
  }

  return uncompressedData;
}

} // namespace pkg

#endif // __Package_h__
