#ifndef __Package_h__
#define __Package_h__

#include <string>
#include "os/MemoryMappedFile.h"
#include "os/Hash.h"
#include "os/Path.h"
#include "os/Queue.h"

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
	inline std::string hashDirectory(const io::Path &path, const io::Path &storagePath, int threadCount=0) {
		typedef std::vector<std::thread> ThreadList;
		ThreadList threads;
		PathQueue filesToProcess;
		HashQueue fileHashes;

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
				filesToProcess.enqueue(entry);
			}
		}

		// TODO: get hashes

	}

}

#endif // __Package_h__
