#include "libernet/Server.h"
#include "os/Thread.h"
#include "os/Environment.h"

#include <stdio.h>

io::Path location() {
	return io::Path(env::get("HOME")) + "Library" + "Caches" + "libernet";
}

int main(int /*argc*/, const char */*argv*/[]) {
	try {
		store::Storage	store(location());
		server::HTTP	server(8080, store);

		while(true) {
			exec::ThreadId::sleep(1.0);
		}
	} catch(const std::exception &exception) {
		fprintf(stderr, "Exception: %s\n", exception.what());
		return 1;
	}
	return 0;
}
