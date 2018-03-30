#include <stdio.h>
#include "libernet/Compute.h"
#include "os/Thread.h"

int main(int argc, const char *argv[]) {
#ifndef __Tracer_h__
exec::ThreadId::sleep(1.05, exec::ThreadId::Seconds);
#endif
	try {
		json::Value 			package;
		std::string				contentsHash;
		std::string				finalHash;
		std::string 			contents;
		std::string				identifier;
		io::Path				location("bin/logs/compute");
		io::Path				outDir("bin/logs/results");
		store::Container		store(location);
		json::Value::StringList	names;

		package.makeObject();
		for (int arg= 1; arg < argc; ++arg) {
			try {
				json::Value		fileInfo;

				identifier= compute::stash(io::Path(argv[arg]).contents(), store, finalHash, contentsHash);
				fileInfo.makeObject();
				fileInfo["identifier"]= identifier;
				fileInfo["sha256"]= finalHash;
				fileInfo["aes256"]= contentsHash;
				package[argv[arg]]= fileInfo;
				printf("%s\t%s\n", argv[arg], identifier.c_str());
			} catch(const std::exception &exception) {
				printf("EXCEPTION: %s\nUnable to process '%s'\n", exception.what(), argv[arg]);
			}
		}

		identifier= compute::stash(package, store, finalHash, contentsHash);
		printf("%s\n", identifier.c_str());
		package.parse(compute::unstash(store, identifier));
		printf("RAW PACKAGE: %s\n", compute::unstash(store, identifier).c_str());
		printf("PACKAGE: %s\n", std::string(package).c_str());
		names= package.keys();
		for (json::Value::StringList::iterator name= names.begin(); name != names.end(); ++name) {
			io::Path	outFile= outDir + *name;

			outFile.parent().mkdirs();
			outFile.write(compute::unstash(store, package[*name]["identifier"].string()));
		}
		printf("%s\n", std::string(package).c_str());
	} catch(const std::exception &exception) {
		printf("EXCEPTION: %s\n", exception.what());
	}
	return 0;
}
