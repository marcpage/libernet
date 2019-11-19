#ifndef __Package_h__
#define __Package_h__

#include <string>
#include "os/MemoryMappedFile.h"
#include "os/Hash.h"

namespace package {

	inline std::string hashFile(const std::string &path) {
		io::MemoryMappedFile	file(path);

		return hash::sha256(file, file.size()).hex();
	}

}

#endif // __Package_h__
