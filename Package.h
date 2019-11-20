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

	inline std::string hashFile(const io::Path &path) {
		io::MemoryMappedFile	file(path);

		return hash::sha256(file, file.size()).hex();
	}
	typedef exec::Queue<io::Path> PathQueue;
	typedef std::pair<io::Path,std::string> PathHash;
	typedef exec::Queue<PathHash> HashQueue;

	inline void _hashingThread(PathQueue &filePathsIn, HashQueue &hashesOut) {
		try {
			while (true) {
				io::Path	next = filePathsIn.dequeue();

				if (next.isEmpty()) {
					filePathsIn.enqueue(next);
					break;
				}

				std::string	hash = hashFile(next);

				hashesOut.enqueue(PathHash(next, hash));
			}
		} catch(const PathQueue::Closed &) {
			// ignore, this is expected
		} catch(const HashQueue::Closed &) {
			// ignore, this is expected
		} catch(const std::exception &exception) {
			fprintf(stderr, "Unexpected exception: %s\n", exception.what());
		}
	}

	inline std::string encryptFile(const io::Path &source, const io::Path &storagePath) {
		io::Path					tempFile = storagePath.uniqueName();
		io::MemoryMappedFile		sourceFile(source, 0, 0, PROT_READ);
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

	inline std::string packageDirectory(const io::Path &path, const io::Path &storagePath, int threadCount=0) {
		typedef std::vector<std::thread> ThreadList;
		ThreadList threads;
		PathQueue filesToProcess;
		HashQueue fileHashes;
		json::Value listing;

		if (0 == threadCount) {
			threadCount = std::thread::hardware_concurrency();
		}
		if (0 == threadCount) {
			threadCount = 2;
		}

		for (int thread = 0; thread < threadCount; ++thread) {
			threads.push_back(std::thread(_hashingThread, std::ref(filesToProcess), std::ref(fileHashes)));
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

		while (!fileHashes.empty()) {
			PathHash next = fileHashes.dequeue();

		}

		return "";
	}

}

#endif // __Package_h__
