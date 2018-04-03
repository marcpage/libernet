#include <stdio.h>
#include "libernet/Compute.h"
#include "os/Thread.h"

int main(int argc, const char *argv[]) {
#ifndef __Tracer_h__
exec::ThreadId::sleep(1.05, exec::ThreadId::Seconds);
#endif
	try {
		io::Path				location("bin/scratch/Compute");

		if (location.exists()) {
			try {
				location.remove();
			} catch(const posix::err::ETIMEDOUT_Errno&) {}
		}

		json::Value 			package;
		std::string				contentsHash;
		std::string				finalHash;
		std::string 			contents;
		std::string				identifier;
		//io::Path				outDir("bin/logs/results");
		store::Storage			store(location);
		json::Value::StringList	names;
		io::Path				currentDirectory(".");
		io::Path::StringList	files= currentDirectory.list(io::Path::PathAndName);

		package.makeObject();
		for (io::Path::StringList::iterator name= files.begin(); name != files.end(); ++name) {
			json::Value		fileInfo;

			try {
				io::Path	item(*name);

				if (item.isFile()) {
					identifier= compute::stash(io::Path(*name).contents(), store, finalHash, contentsHash);
					fileInfo.makeObject();
					fileInfo["identifier"]= identifier;
					fileInfo["sha256"]= finalHash;
					fileInfo["aes256"]= contentsHash;
					package[*name]= fileInfo;
				}
			} catch(const std::exception &exception) {
				printf("FAILED: EXCEPTION: %s\nUnable to process '%s'\n", exception.what(), name->c_str());
			}
		}
		for (int arg= 1; arg < argc; ++arg) {
			try {
				json::Value		fileInfo;

				identifier= compute::stash(io::Path(argv[arg]).contents(), store, finalHash, contentsHash);
				fileInfo.makeObject();
				fileInfo["identifier"]= identifier;
				fileInfo["sha256"]= finalHash;
				fileInfo["aes256"]= contentsHash;
				package[argv[arg]]= fileInfo;
				//printf("%s\t%s\n", argv[arg], identifier.c_str());
			} catch(const std::exception &exception) {
				printf("FAILED: EXCEPTION: %s\nUnable to process '%s'\n", exception.what(), argv[arg]);
			}
		}

		identifier= compute::stash(package, store, finalHash, contentsHash);
		printf("%s\n", identifier.c_str());
		package.parse(compute::unstash(store, identifier));
		names= package.keys();
		for (json::Value::StringList::iterator name= names.begin(); name != names.end(); ++name) {
			identifier= package[*name]["identifier"].string();
			//io::Path	outFile= outDir + *name;
			std::string	data= compute::unstash(store, identifier);
			if(hash::sha256(data).hex() != identifier.substr(identifier.find(':') + 1)) {
				printf("FAILED: contents were corrupted: %s\n", name->c_str());
			}
			//outFile.parent().mkdirs();
			//outFile.write(compute::unstash(store, package[*name]["identifier"].string()));
		}
		printf("%s\n", std::string(package).c_str());
	} catch(const std::exception &exception) {
		printf("FAILED: EXCEPTION: %s\n", exception.what());
	}
	return 0;
}
