#include "libernet/Package.h"
#include "os/Path.h"
#include "os/Hash.h"
#include "os/Execute.h"

#define dotest(condition) \
	if(!(condition)) { \
		fprintf(stderr, "FAIL(%s:%d): %s\n",__FILE__, __LINE__, #condition); \
	}

int main(const int argc, const char * const argv[]) {
	int	iterations= 2;
#ifdef __Tracer_h__
	iterations= 1;
#endif
	const io::Path	kTestPath(argc < 2 ? "bin/test_files" : argv[1]);
	const io::Path	kStoragePath = io::Path("bin/logs").uniqueName("Package_test");
	std::string		identifier;

	if (!kStoragePath.isDirectory()) {
		kStoragePath.mkdir();
	}

	if (argc < 2) {
		io::Path headers = kTestPath + "headers";
		io::Path sources = kTestPath + "sources";

		if (!kTestPath.isDirectory()) {
			kTestPath.mkdir();
		}
		if (!headers.isDirectory()) {
			headers.mkdir();
		}
		if (!sources.isDirectory()) {
			sources.mkdir();
		}
		exec::execute("cp *.h '" + std::string(headers) + "'");
		exec::execute("cp tests/*.cpp '" + std::string(sources) + "'");
		exec::execute("touch '" + std::string(kTestPath + "manifest") + "'");
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
