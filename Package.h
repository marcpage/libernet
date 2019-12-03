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

	inline std::string storeData(const std::string &sourceData, const io::Path &storagePath) {
		hash::sha256		sourceHash(sourceData);
		std::string			compressedData = z::compress(sourceData, 9);
		const std::string	&dataToEncrypt = (sourceData.size() > compressedData.size()) ? compressedData : sourceData;
		std::string			key(reinterpret_cast<const char*>(sourceHash.buffer()), sourceHash.size());
		std::string			encrypted = crypto::AES256(key).encrypt(dataToEncrypt);
		std::string			dataName = hash::sha256(encrypted).hex();

		(storagePath + dataName).write(encrypted);

		return "sha256:" + dataName + ":" + "aes256/sha256:" + sourceHash.hex();
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

	inline std::string packageDirectory(const io::Path &path, const io::Path &storagePath) {
		json::Value 	listing = json::Value().makeObject();
		const io::Path	storagePathAbsolute = storagePath.absolute();
		const io::Path	listingPathAbsolute = path.absolute();

		io::Path::StringList contents = listingPathAbsolute.list(io::Path::PathAndName, io::Path::RecursiveListing);

		listing["directories"] = json::Value().makeArray();
		listing["files"] = json::Value().makeObject();
		for (auto entry = contents.begin(); entry != contents.end(); ++entry) {
			if (!io::Path::endsWithPathSeparator(*entry)) {
				listing["files"][io::Path(*entry).relativeTo(listingPathAbsolute)] = encryptFile(*entry, storagePathAbsolute);
			} else {
				json::Value directoryName;
				directoryName = std::string(io::Path(*entry).relativeTo(listingPathAbsolute));
				listing["directories"].append(directoryName);
			}
		}

		return storeData(listing, storagePathAbsolute);
	}

}

#endif // __Package_h__
