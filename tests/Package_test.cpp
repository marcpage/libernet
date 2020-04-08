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
		}

		json::Value		original = pkg::directoryInfo(kTestPath, kStoragePath);
		std::string		packedData;
		std::string nameType, name, keyType, key;

		pkg::splitIdentifier(identifier, nameType, name, keyType, key);

		if (nameType != "sha256") {
			printf("FAIL: expected nameType to be sha256 but got '%s' for '%s'\n", nameType.c_str(), identifier.c_str());
		}

		if (keyType != "aes256/sha256") {
			printf("FAIL: expected keyType to be aes256/sha256 but got '%s' for '%s'\n", keyType.c_str(), identifier.c_str());
		}

		printf("identifier = %s\noriginal\n%s\n", identifier.c_str(), original.format(4).c_str());
		pkg::packData(original, packedData);
		printf("directory data size = %lu packed size = %lu\n", std::string(original).size(), packedData.size());

		json::Value unpacked(pkg::unpackData(packedData, identifier));
		printf("original\n%s\n", unpacked.format(4).c_str());

	} catch(const std::exception &exception) {
		printf("FAIL: Exception thrown: %s\n", exception.what());
	}

	return 0;
}
