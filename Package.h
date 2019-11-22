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
			static const std::string emptyHash = hash::sha256("").hex();

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

	typedef exec::Queue<io::Path> PathQueue;
	typedef std::pair<io::Path,std::string> PathId;
	typedef exec::Queue<PathId> IdQueue;

	inline void _encryptFileThread(PathQueue &filePathsIn, IdQueue &idsOut, const io::Path &storagePath) {
		try {
			while (true) {
				io::Path	next = filePathsIn.dequeue();

				if (next.isEmpty()) {
					filePathsIn.enqueue(next);
					break;
				}

				std::string	identifier = encryptFile(next, storagePath);

				idsOut.enqueue(PathId(next, identifier));
			}
		} catch(const PathQueue::Closed &) {
			// ignore, this is expected
		} catch(const IdQueue::Closed &) {
			// ignore, this is expected
		} catch(const std::exception &exception) {
			fprintf(stderr, "Unexpected exception: %s\n", exception.what());
		}
	}

	inline std::string packageDirectory(const io::Path &path, const io::Path &storagePath, int threadCount=0) {
		json::Value listing = json::Value().makeObject();
		IdQueue fileIds;
/*
		typedef std::vector<std::thread> ThreadList;
		ThreadList threads;
		PathQueue filesToProcess;

		if (0 == threadCount) {
			threadCount = std::thread::hardware_concurrency();
		}
		if (0 == threadCount) {
			threadCount = 2;
		}

		for (int thread = 0; thread < threadCount; ++thread) {
			threads.push_back(std::thread(_encryptFileThread, std::ref(filesToProcess), std::ref(fileIds), std::ref(storagePath)));
		}
*/
		io::Path::StringList contents = path.list(io::Path::PathAndName, io::Path::RecursiveListing);

		listing["directories"] = json::Value().makeArray();
		for (auto entry = contents.begin(); entry != contents.end(); ++entry) {
			if (!io::Path::endsWithPathSeparator(*entry)) {
/*
				filesToProcess.enqueue(*entry);
*/
				fileIds.enqueue(PathId(*entry, encryptFile(*entry, storagePath))); // single threaded version
			} else {
				json::Value directoryName = std::string(io::Path(*entry).relativeTo(path));
				listing["directories"].append(directoryName);
			}
		}

/*
		filesToProcess.enqueue(io::Path());

		for (auto thread = threads.begin(); thread != threads.end(); ++thread) {
			thread->join();
		}
*/
		listing["files"] = json::Value();
		while (!fileIds.empty()) {
			PathId next = fileIds.dequeue();

			listing["files"][next.first.relativeTo(path)] = next.second;
		}

		return storeData(listing, storagePath);
	}

}

#endif // __Package_h__
