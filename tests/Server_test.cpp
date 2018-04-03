#include "libernet/Server.h"

int main(int /*argc*/, char */*argv*/[]) {
	store::Storage	store(io::Path("bin/scratch/Server"));
	server::HTTP	server(8123, store);
	http::Request	request;

	//server.join();
	return 0;
}
