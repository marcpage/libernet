#include "libernet/Package.h"
#include "os/Path.h"
#include "os/Hash.h"

#define dotest(condition) \
	if(!(condition)) { \
		fprintf(stderr, "FAIL(%s:%d): %s\n",__FILE__, __LINE__, #condition); \
	}

int main(const int argc, const char * const argv[]) {
	int	iterations= 40;
#ifdef __Tracer_h__
	iterations= 1;
#endif
	const io::Path	kTestFilePath(argc < 2 ? "bin/logs/testMemoryMappedFile.txt" : argv[1]);
	const char * const kTestFileContents = "Testing hashing file.";

	for (int i = 0; i < iterations; ++i) {
		if (kTestFilePath.isFile()) {
			kTestFilePath.remove();
		}

		kTestFilePath.write(kTestFileContents);
		printf("%s size=%lu\n", kTestFileContents, strlen(kTestFileContents));
		std::string dataHash = hash::sha256(kTestFileContents, strlen(kTestFileContents)).hex();
		std::string fileHash = package::hashFile(kTestFilePath);
		dotest(dataHash == fileHash);
	}

	return 0;
}
