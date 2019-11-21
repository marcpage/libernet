#ifndef __Package_h__
#define __Package_h__

#include "os/MemoryMappedFile.h"
#include "os/Hash.h"
#include "os/Path.h"
#include "os/Queue.h"
#include "os/SymetricEncrypt.h"
#include "protocol/JSON.h"
#include <thread>
#include <string>

namespace package {

	inline std::string encryptFilePart(const io::Path &source, const off_t offset, const size_t size, const io::Path &storagePath) {
		io::Path					tempFile = storagePath.uniqueName();
		io::MemoryMappedFile		sourceFile(source, offset, size, PROT_READ);
		io::FileDescriptor			destinationFile(tempFile);
		size_t						encryptedSize = source.size() + crypto::AES256_CBC_Padded::BlockSize;

		destinationFile.resize(encryptedSize);

		io::MemoryMappedFile		destination(destinationFile);
		hash::sha256				sourceHash(sourceFile, sourceFile.size());
		std::string					key(reinterpret_cast<const char*>(sourceHash.buffer()), sourceHash.size());
		crypto::AES256_CBC_Padded	cryptor(key);

		cryptor.encryptInPlace(sourceFile.address<char>(), sourceFile.size(), "", destination.address<char>(), encryptedSize);
		sourceFile.close();
		destination.close();
		destinationFile.resize(encryptedSize);

		io::MemoryMappedFile		encryptedFile(tempFile);
		hash::sha256				encryptedHash(encryptedFile, encryptedFile.size());

		tempFile.rename(storagePath + encryptedHash.hex());
		return "sha256:" + encryptedHash.hex() + ":" + "aes256/sha256:" + sourceHash.hex();
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
		typedef std::vector<std::thread> ThreadList;
		ThreadList threads;
		PathQueue filesToProcess;
		IdQueue fileIds;
		json::Value listing;

		if (0 == threadCount) {
			threadCount = std::thread::hardware_concurrency();
		}
		if (0 == threadCount) {
			threadCount = 2;
		}

		for (int thread = 0; thread < threadCount; ++thread) {
			threads.push_back(std::thread(_encryptFileThread, std::ref(filesToProcess), std::ref(fileIds), std::ref(storagePath)));
		}

		io::Path::StringList contents = path.list(io::Path::PathAndName, io::Path::RecursiveListing);

		for (auto entry = contents.begin(); entry != contents.end(); ++entry) {
			if (!io::Path::endsWithPathSeparator(*entry)) {
				filesToProcess.enqueue(*entry);
			} else {
				listing["directories"] = *entry; // TODO this needs to be relative
			}
		}

		filesToProcess.enqueue(io::Path());

		for (auto thread = threads.begin(); thread != threads.end(); ++thread) {
			thread->join();
		}

		while (!fileIds.empty()) {
			PathId next = fileIds.dequeue();

			listing[next.first] = next.second;
		}

		// TODO: Create a compressed, encrypted file from listing and return the identifier

		return "";
	}

}

#endif // __Package_h__
