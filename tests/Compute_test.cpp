#include "libernet/Compute.h"
#include "os/Thread.h"

int main(int argc, const char *argv[]) {
#ifndef __Tracer_h__
exec::ThreadId::sleep(1.05, exec::ThreadId::Seconds);
#endif
	for (int arg= 1; arg < argc; ++arg) {
		try {
			std::string 	contents= io::Path(argv[arg]).contents();
			hash::sha256 	contentsHash(contents);
			crypto::AES256	key(contentsHash.data());
			std::string		compressed= z::compress(contents, 9);
			std::string		encrypted= key.encrypt(compressed);
			hash::sha256 	finalHash(encrypted);
			printf("%s:%s\t%s\n", finalHash.hex().c_str(), contentsHash.hex().c_str(), argv[arg]);
		} catch(const std::exception &exception) {
			printf("EXCEPTION: %s\nUnable to process '%s'\n", exception.what(), argv[arg]);
		}
	}
	return 0;
}
