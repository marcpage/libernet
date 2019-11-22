#include "libernet/Package.h"
#include "os/Path.h"
#include "os/Hash.h"

#define dotest(condition) \
	if(!(condition)) { \
		fprintf(stderr, "FAIL(%s:%d): %s\n",__FILE__, __LINE__, #condition); \
	}

int main(const int argc, const char * const argv[]) {
	int	iterations= 2;
#ifdef __Tracer_h__
	iterations= 1;
#endif
	const io::Path	kTestPath(argc < 2 ? "." : argv[1]);
	const io::Path	kStoragePath = io::Path("/tmp").uniqueName();
	std::string		identifier;

	if (!kStoragePath.isDirectory()) {
		kStoragePath.mkdir();
	}

	try {
		for (int i = 0; i < iterations; ++i) {
			identifier = pkg::packageDirectory(kTestPath, kStoragePath);
		}
	} catch(const std::exception &exception) {
		printf("FAIL: Exception thrown: %s\n", exception.what());
	}

	printf("identifier = %s\n", identifier.c_str());

	return 0;
}
