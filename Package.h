#ifndef __Package_h__
#define __Package_h__

#include "os/MemoryMappedFile.h"
#include "os/Hash.h"
#include "os/Path.h"
#include "os/Queue.h"
#include "os/SymetricEncrypt.h"
#include "os/ZCompression.h"
#include "protocol/JSON.h"
#include <thread>
#include <string>

namespace pkg {

	inline void splitIdentifier(const std::string &identifier, std::string &nameType, std::string &name, std::string &keyType, std::string &key) {
		const std::string::size_type firstColon = identifier.find(':');
		const std::string::size_type secondColon = identifier.find(':', firstColon + 1);
		const std::string::size_type thirdColon = identifier.find(':', secondColon + 1);

		nameType.clear();
		name.clear();
		keyType.clear();
		key.clear();
		if (std::string::npos != firstColon) {
			nameType.assign(identifier, 0, firstColon);
			if (std::string::npos != secondColon) {
				name.assign(identifier, firstColon + 1, secondColon);
				if (std::string::npos != secondColon) {
					keyType.assign(identifier, secondColon + 1, thirdColon);
					key.assign(identifier, thirdColon + 1);
				}
			}
		}
	}

	inline std::string packData(const std::string &sourceData, std::string &packedData, std::string &dataName) {
		hash::sha256		sourceHash(sourceData);
		std::string			compressedData = z::compress(sourceData, 9);
		const std::string	&dataToEncrypt = (sourceData.size() > compressedData.size()) ? compressedData : sourceData;
		std::string			key(reinterpret_cast<const char*>(sourceHash.buffer()), sourceHash.size());

		packedData = crypto::AES256(key).encrypt(dataToEncrypt);
		dataName = hash::sha256(packedData).hex();
		return "sha256:" + dataName + ":" + "aes256/sha256:" + sourceHash.hex();
	}

	inline std::string packData(const std::string &sourceData, std::string &packedData) {
		std::string dataName;

		return packData(sourceData, packedData, dataName);
	}

	inline std::string storeData(const std::string &sourceData, const io::Path &storagePath) {
		std::string			packedData, dataName;
		std::string			identifier = packData(sourceData, packedData, dataName);

		(storagePath + dataName).write(packedData);
		return identifier;
	}

	inline std::string encryptFilePart(const io::Path &source, const off_t offset, const size_t size, const io::Path &storagePath) {
		return storeData(source.contents(io::File::Binary, offset, size), storagePath);
	}

	inline std::string encryptFile(const io::Path &source, const io::Path &storagePath) {
		const off_t fileSize = source.size();
		const size_t filePartSize = 1024 * 1024; // 1 MiB max part size

		if (0 == fileSize) {
			// e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
			static const std::string emptyHash = hash::sha256("", 0).hex();

			return "sha256:" + emptyHash;
		}

		const int		parts = (fileSize - 1) / filePartSize + 1;
		const size_t	lastPartSize = fileSize - (parts - 1) * filePartSize;
		std::string		identifier;
		std::string		prefix;

		for (int i = 0; i < parts; ++i) {
			const size_t partSize = (i == parts - 1) ? lastPartSize : filePartSize;
			const std::string partIdentifier = encryptFilePart(source, i * filePartSize, partSize, storagePath);

			identifier = identifier + prefix + partIdentifier;
			prefix = ",";
		}

		return identifier;
	}

	typedef std::pair<io::Path,std::string> PathId;
	typedef exec::Queue<PathId> IdQueue;

	inline json::Value directoryInfo(const io::Path &path, const io::Path &storagePath) {
		json::Value 	listing = json::Value(json::ObjectType);
		const io::Path	storagePathAbsolute = storagePath.absolute();
		const io::Path	listingPathAbsolute = path.absolute();

		io::Path::StringList contents = listingPathAbsolute.list(io::Path::PathAndName, io::Path::RecursiveListing);

		for (auto entry = contents.begin(); entry != contents.end(); ++entry) {
			if (!io::Path::endsWithPathSeparator(*entry)) {
				listing["files"][io::Path(*entry).relativeTo(listingPathAbsolute)]["contents"] = encryptFile(*entry, storagePathAbsolute);
				listing["files"][io::Path(*entry).relativeTo(listingPathAbsolute)]["size"] = io::Path(*entry).size();
			} else {
				json::Value directoryName;
				directoryName = std::string(io::Path(*entry).relativeTo(listingPathAbsolute));
				listing["directories"].append(directoryName);
			}
		}

		return listing;
	}

	inline std::string packageDirectory(const io::Path &path, const io::Path &storagePath) {
		return storeData(directoryInfo(path, storagePath), storagePath.absolute());
	}

	inline json::Value unpackageDirectoryInfo(const std::string &packagedDirectory, const std::string &identifier = std::string()) {
		try {
			return json::Value(packagedDirectory);
		} catch(const json::WrongType &) {
			std::string	nameType, name, keyType, key;

			splitIdentifier(identifier, nameType, name, keyType, key);
			if (nameType != "sha256") {
				throw json::WrongType("Expected data identifier to be sha256, but found '" + nameType + "'", __FILE__, __LINE__);
			} else if ( (keyType.size() > 0) && (keyType != "aes256/sha256") ) {
				throw json::WrongType("Expected key type to be aes256/sha256 or blank, but found '" + keyType + "'", __FILE__, __LINE__);
			} else if ( (keyType.size() == 0) && (key.size() != 0) ) {
				throw json::WrongType("Expected empty key if keyType is empty, but found key: '" + key + "'", __FILE__, __LINE__);
			} else if (key.size() == 0) {
				throw json::WrongType("We have an empty key, but a key type of: '" + keyType + "'", __FILE__, __LINE__);
			}
			// verify hash of packagedDirectory matches sha256
			// if key is empty, try to json parse the data and return it, else
			// create key from key string
			// decrypt data
			// try to json parse data and return it, else
			// decompress data, json parse, and return it

		}

	}


}

#endif // __Package_h__
