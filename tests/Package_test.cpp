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
	json::Value		source;

	kStoragePath.mkdirs();

	if (argc < 2) {
		io::Path headers = kTestPath + "headers";
		io::Path sources = kTestPath + "sources";

		kTestPath.mkdirs();
		headers.mkdirs();
		sources.mkdirs();

		try {
			exec::execute("cp *.h '" + std::string(headers) + "'");
			exec::execute("cp tests/*.cpp '" + std::string(sources) + "'");
			exec::execute("touch '" + std::string(kTestPath + "manifest") + "'");
		} catch(const std::exception &exception) {
			printf("FAIL: exception thrown during source prep: %s\n", exception.what());
		}
	}

	try {
		for (int i = 0; i < iterations; ++i) {
			identifier = pkg::packageDirectory(kTestPath, kStoragePath);
			source = pkg::directoryInfo(kTestPath, kStoragePath);
		}
		printf("identifier = %s\nsource\n%s\n", identifier.c_str(), source.format(4).c_str());
	} catch(const std::exception &exception) {
		printf("FAIL: Exception thrown: %s\n", exception.what());
	}

	return 0;
}
