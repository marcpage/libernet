#include "os/AddressIPv4.h"
#include "os/BufferAddress.h"
#include "os/BufferString.h"
#include "os/Hash.h"
#include "os/Path.h"
#include "os/Socket.h"
#include "os/SymetricEncrypt.h"
#include "os/Thread.h"
#include "os/ZCompression.h"
#include "protocol/HTTP.h"

#include <stdio.h>

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

std::string sendFile(const io::Path &path, int port) {
	std::string			contents = path.contents();
	hash::sha256		keyData(contents);
	std::string			compressed= z::compress(contents, 9);
	std::string			key(reinterpret_cast<const char*>(keyData.buffer()), keyData.size());
	crypto::AES256 		cryptor(key);
	std::string			encrypted = crypto::AES256(key).encrypt(compressed);
	std::string			name = hash::sha256(encrypted).hex();
	std::string			identifier = "/sha256/" + name + "/aes256/" + keyData.hex();

	try {
		std::string			output;
		net::AddressIPv4	localhost(port);
		net::Socket			connection(localhost.family());

		connection.connect(localhost);

		http::Response		response= sendRequest(connection, "/sha256/" + name, output, "PUT", encrypted);

		try {
			AssertMessageException(response.info().code() == "200");
		} catch(const std::exception &exception) {
			fprintf(stderr, "%s\n", std::string(response).c_str());
			throw;
		}
		connection.close();
	} catch(const std::exception &exception) {
		fprintf(stderr, "Path = %s\n", std::string(path).c_str());
		throw;
	}
	return identifier;
}

int main(int argc, const char *argv[]) {
	try {
		for (int i = 1; i < argc; ++i) {
			io::Path	path(argv[i]);

			try {
				printf("Sending %s\n", argv[i]);
				printf("%s\t%s\n", argv[i], sendFile(path, 8080).c_str());
			} catch(const std::exception &exception) {
				fprintf(stderr, "Exception sending '%s': %s\n", argv[i], exception.what());
			}
		}
	} catch(const std::exception &exception) {
		fprintf(stderr, "Exception: %s\n", exception.what());
		return 1;
	}
	return 0;
}
