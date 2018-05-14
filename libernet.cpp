#include "libernet/Server.h"
#include "libernet/Logger.h"

#include "os/Thread.h"
#include "os/Environment.h"

#include <stdio.h>

io::Path location() {
	return io::Path(env::get("HOME")) + "Library" + "Caches" + "libernet";
}

int main(int /*argc*/, const char */*argv*/[]) {
	logger::Logger	log(stderr);//io::Path(env::get("HOME")) + "Library" + "Logs" + "libernet.txt");

	try {
		store::Storage	store(location());
		server::HTTP	server(8080, store, log);

		while(true) {
			exec::ThreadId::sleep(1.0);
		}
	} catch(const std::exception &exception) {
		fprintf(stderr, "Exception: %s\n", exception.what());
		return 1;
	}
	log.finish();
	return 0;
}
