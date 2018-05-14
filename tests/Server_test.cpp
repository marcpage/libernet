#include "libernet/Server.h"

std::string readline(net::Socket &connection) {
	char			byte= '\0';
	BufferAddress	singleByte(&byte, 1);
	std::string		buffer;

	while (byte != '\n') {
		connection.read(singleByte, 1);
		buffer += byte;
	}
	return buffer;
}

http::Response sendRequest(net::Socket &connection, const std::string &path, std::string &output, const std::string &method="GET", const std::string &data="") {
	http::Request	message;
	std::string		header;
	std::string		line;
	http::Response	response;

	output.clear();
	message.info().method()= method;
	if (data.length() > 0) {
		message.fields()["Content-Length"]= std::to_string(data.length());
	}
	message.info().path()= path;
	line= std::string(message);
	connection.write(BufferString(line));
	if (data.length() > 0) {
		line= data;
		connection.write(BufferString(line));
	}
	do {
		line= readline(connection);
		header+= line;
	} while( (line != "") && (line != "\r\n") && (line != "\n") );
	response= http::Response(header);
	if (response.fields().has("Content-Length")) {
		long			size= std::stol(response.fields()["Content-Length"]);
		std::string		readData(size, '\0');
		BufferString	dataBuffer(readData);

		do {
			size_t amount= connection.read(dataBuffer, size);

			output.append(readData, 0, amount);
			size-= amount;
		} while(size > 0);
	}
	printf("%p REQUEST\n-------\n%s\n-----------\n%s\n========================\n", exec::ThreadId::current().thread(), std::string(message).c_str(), data.c_str());
	printf("%p RESPONSE\n-------\n%s\n-----------\n%s\n========================\n", exec::ThreadId::current().thread(), std::string(response).c_str(), output.c_str());
	return response;
}

int main(int /*argc*/, char */*argv*/[]) {
	try {
		store::Storage		store(io::Path("bin/scratch/Server"));
		logger::Logger		log(io::Path("bin/scratch/Server/log.txt"));
		server::HTTP		server(8123, store, log);
		net::AddressIPv4	localhost(8123);
		net::Socket			connection(localhost.family());
		std::string			data= "Testing";
		std::string			compressedData= z::compress(data, 9);
		hash::sha256		hash(data);
		http::Response		response;
		std::string			output;

		connection.connect(localhost);

		printf("%p Sending Data\n", exec::ThreadId::current().thread());
		response= sendRequest(connection, "/sha256/" + hash.hex(), output, "PUT", compressedData);
		if (response.info().code() != "200") {
			printf("FAIL: PUT code should be 200, but its %s\n", response.info().code().c_str());
		}

		printf("%p Receiving Data\n", exec::ThreadId::current().thread());
		response= sendRequest(connection, "/sha256/" + hash.hex(), output, "GET");
		if (response.info().code() != "200") {
			printf("FAIL: GET code should be 200, but its %s\n", response.info().code().c_str());
		}

		printf("%p Sending Bad Data\n", exec::ThreadId::current().thread());
		response= sendRequest(connection, "/sha256/" + hash.hex(), output, "PUT", "Bogus");
		if (response.info().code() != "415") {
			printf("FAIL: Bad PUT code should be 415, but its %s\n", response.info().code().c_str());
		}

		printf("%p Sending Data to bad address\n", exec::ThreadId::current().thread());
		response= sendRequest(connection, "/sha256/" + hash.hex() + "/aes256/" + hash.hex(), output, "PUT", compressedData);
		if (response.info().code() != "404") {
			printf("FAIL: PUT code should be 404, but its %s\n", response.info().code().c_str());
		}

		printf("%p Sending Data with bad method\n", exec::ThreadId::current().thread());
		response= sendRequest(connection, "/sha256/" + hash.hex(), output, "What", compressedData);
		if (response.info().code() != "404") {
			printf("FAIL: PUT code should be 404, but its %s\n", response.info().code().c_str());
		}

		printf("%p Receiving Data with bad method\n", exec::ThreadId::current().thread());
		response= sendRequest(connection, "/sha256/" + hash.hex(), output, "What");
		if (response.info().code() != "404") {
			printf("FAIL: PUT code should be 404, but its %s\n", response.info().code().c_str());
		}

		printf("%p Receiving Data from bad address\n", exec::ThreadId::current().thread());
		response= sendRequest(connection, "/sha256/L000000000000000000000000000000000000000000000000000000000000000", output, "GET");
		if (response.info().code() != "404") {
			printf("FAIL: GET code should be 404, but its %s\n", response.info().code().c_str());
		}

		printf("%p Receiving Data\n", exec::ThreadId::current().thread());
		response= sendRequest(connection, "/sha256/" + hash.hex() + "/aes256/" + hash.hex() + "/index.html", output, "GET");
		if (response.info().code() != "400") {
			printf("FAIL: GET code should be 400, but its %s\n", response.info().code().c_str());
		}

		log.finish();
	} catch(const std::exception &e) {
		printf("FAILED: Exception: %s\n", e.what());
	}
	//server.join();
	return 0;
}
