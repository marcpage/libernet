#ifndef __Compute_h__
#define __Compute_h__

#include "os/Hash.h"
#include "os/Path.h"
#include "os/ZCompression.h"
#include "os/SymetricEncrypt.h"
#include "protocol/JSON.h"
#include "libernet/Container.h"

namespace compute {

	std::string stash(const std::string &contents, store::Container &container, std::string &identifier, std::string &key) {
		hash::sha256 	contentsHash(contents);
		crypto::AES256	cryptor(contentsHash.data());
		std::string		compressed= z::compress(contents, 9);
		std::string		encrypted= cryptor.encrypt(compressed);
		hash::sha256	finalHash(encrypted);

		container.put(finalHash.hex(), encrypted);
		return finalHash.hex(identifier) + ":" + contentsHash.hex(key);
	}

	std::string unstash(store::Container &store, const::std::string &identifier) {
		std::string		encrypted= store.get(identifier.substr(0, identifier.find(':')));
		hash::sha256	contentsHash= hash::sha256::fromHex(identifier.substr(identifier.find(':') + 1));
		crypto::AES256	cryptor(contentsHash.data());
		std::string		compressed= cryptor.decrypt(encrypted);
		std::string		contents= z::uncompress(compressed, 2 * 1024 * 1024);

		return contents;
	}

}

#endif // __Computer_h__
