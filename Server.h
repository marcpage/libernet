#ifndef __Server_h__
#define __Server_h__

#include "os/Queue.h"
#include "os/Thread.h"
#include "os/Hash.h"
#include "os/SymetricEncrypt.h"
#include "os/SocketServer.h"
#include "os/AddressIPv4.h"
#include "os/AddressIPv6.h"
#include "os/BufferManaged.h"
#include "os/BufferString.h"
#include "os/ZCompression.h"
#include "protocol/HTTP.h"
#include "protocol/JSON.h"
#include "libernet/Storage.h"
#include "libernet/Compute.h"
#include <vector>

namespace server {

	class HTTPHandler : public exec::Thread {
		public:
			HTTPHandler(store::Storage &store);
			virtual ~HTTPHandler();
			bool handleConnection(net::Socket *connection);
		protected:
			virtual void *run();
		private:
			store::Storage				&_store;
			bool						_working;
			exec::Queue<net::Socket*>	_queue;
			http::Request _readRequest(net::Socket &connection);
			std::string &_readLine(net::Socket &connection, std::string &buffer);
			bool _isDataPath(const std::string &path, std::string &name, std::string &key, std::string &suffix);
			std::string _getData(const std::string &name, const std::string &key, const std::string &suffix, std::string &contentType);
	};

	class HTTP : public exec::Thread {
		public:
			HTTP(int port, store::Storage &storage);
			virtual ~HTTP();
		protected:
			virtual void *run();
		private:
			typedef std::vector<HTTPHandler*>	_HandlerList;
			store::Storage	&_store;
			int				_port;
			_HandlerList	_handlers;
	};

	inline HTTPHandler::HTTPHandler(store::Storage &store):exec::Thread(KeepAroundAfterFinish),_store(store),_working(false),_queue() {
		start();
	}
	inline HTTPHandler::~HTTPHandler() {
	}
	inline bool HTTPHandler::handleConnection(net::Socket *connection) {
		if (_queue.size() == 0) {
			_queue.enqueue(connection);
			return true;
		}
		return false;
	}
	inline void *HTTPHandler::run() {
		while (true) {
			_working= false;
			net::Socket		*next= _queue.dequeue();

			try {
				_working= true;
				http::Request		request= _readRequest(*next);
				http::Response		response;
				std::string			buffer;
				const std::string	hash= "/sha256/";
				std::string			responseData;
				std::string			name, key, suffix, contentType;

				responseData= std::string("<html><head><title>404 Path not found</title></head><body><h1>404 Path not found</h1></br><pre>") + std::string(request) + "</pre></body></html>\n";
				response.info().code()= "404";
				response.info().message()= "Not Found";
				response.fields()["Content-Type"]= "text/html; charset=utf-8";
				try {
					if (_isDataPath(request.info().path(), name, key, suffix)) {
						if (request.info().method() == "GET") {
							std::string contents= _getData(name, key, suffix, contentType);

							if (contents.length() > 0) {
								responseData= contents;
								response.info().code()= "200";
								response.info().message()= "OK";
								if (contentType.length() > 0) {
									response.fields()["Content-Type"]= contentType;
								}
							}
						} else if ( (request.info().method() == "PUT") && (request.has("Content-Length"))) && (key.size() == 0) && (suffix.size() == 0)) {
							long			size= std::stol(request["Content-Length"]);
							std::string		data(size, '\0');
							std::string		results;
							BufferString	buffer(data);
							bool			valid;
							std::string		hash;
							std::string		uncompressedHash;
							
							do {
								size_t amount= connection.read(buffer, size);
								
								results.append(data, 0, amount);
								size-= amount;
							} while(size > 0);
							hash= hash::sha256(results).hex();
							valid= hash::sha256(results) == hash::sha256::fromHex(name);
							if (!valid) {
								try {
									uncompressedHash= hash::sha256(z::uncompress(content, 2 * 1024 * 1024)).hex();
									hash::sha256(z::uncompress(content, 2 * 1024 * 1024)) == hash::sha256::fromHex(name);
									valid= true;
							} catch(const z::Exception &) {}
							if (valid) {
								_store.put(name, results);
								response.info().code()= "200";
								response.info().message()= "OK";
								responseData= "<html><head><title>Content Added</title></head><body><h1>Content Added</h1><br/>";
							} else {
								response.info().code()= "415";
								response.info().message()= "Unsupported Media Type";
								responseData= "<html><head><title>415 Unsupported Media Type</title></head><body><h1>415 Unsupported Media Type</h1><br/>";
								responseData= responseData + "Request: " + name + "<br/>";
								responseData= responseData + "Hash: " + hash + "<br/>";
								responseData= responseData + "Uncompressed Hash: " + uncompressedHash + "<br/>";
							}
							responseData= responseData + "</body></html>\n";
						} else {
							if ( (key.size() > 0) || (suffix.size() > 0) ) {
								response.info().code()= "404";
								response.info().message()= "Not Found";
								responseData= "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1><br/>";
							} else {
								response.info().code()= "405";
								response.info().message()= "Method Not Allowed";
								responseData= "<html><head><title>405 Method Not Allowed</title></head><body><h1>405 Method Not Allowed</h1><br/>";
							}
							responseData= responseData + "Method: " + request.info().method() + "<br/>";
							if (request.has("Content-Length")) {
								responseData= responseData + "Size: " + request["Content-Length"] + "<br/>";
							} else {
								responseData= responseData + "Content-Length not specified<br/>";
							}
							if (key.size() > 0) {
								responseData= responseData + "Cannot Specified Key (" + key + ")<br/>";
							}
							if (suffix.size() > 0) {
								responseData= responseData + "Cannot Specified suffix (" + suffix + ")<br/>";
							}
							responseData= responseData + "</body></html>\n";
						}
					}
				} catch(const std::exception &exception) {
					response.info().code()= "400";
					response.info().message()= "Bad Request";
					responseData= std::string("<html><head><title>400 Bad Request</title></head><body><h1>400 Bad Request</h1><br/><pre>") + exception.what() + "</pre><br/>Request:<br/><pre>" + std::string(request) + "</pre></body></html>\n";
				}
				
				if (responseData.size() > 0) {
					response["Content-Length"]= std::to_string(responseData.size());
				}
				buffer= response;
				next->write(BufferString(buffer), buffer.size());
				next->write(BufferString(responseData), responseData.size());
				next->close();
				delete next;
			} catch(const std::exception &exception) {
				// TODO: log
			}
		}
	}
	inline http::Request HTTPHandler::_readRequest(net::Socket &connection) {
		std::string headers, line;

		while ( (_readLine(connection, line) != "\r\n") && (line != "\n") ) {
			headers+= line;
		}
		return http::Request(headers);
	}
	inline std::string &HTTPHandler::_readLine(net::Socket &connection, std::string &buffer) {
			char			byte= '\0';
			BufferAddress	singleByte(&byte, 1);

			buffer.clear();
			while (byte != '\n') {
				connection.read(singleByte, 1);
				buffer += byte;
			}
			return buffer;
	}
	inline bool HTTPHandler::_isDataPath(const std::string &path, std::string &name, std::string &key, std::string &suffix) {
		const std::string				namePrefix("/sha256/");
		const std::string::size_type	nameSize= 32;

		name.clear();
		key.clear();
		suffix.clear();
		if ( (path.length() >= namePrefix.length() + nameSize) && (path.find(namePrefix) == 0) ) {
			const std::string				keyPrefix("/aes256/");
			const std::string::size_type	keySize= 32;

			name= path.substr(keyPrefix.length(), nameSize);
			try {
				hash::sha256::fromHex(name);
			} catch(const msg::Exception &) {
				return false;
			}
			if ( (path.length() >= namePrefix.length() + nameSize + keyPrefix.length() + keySize)
					&& (path.find(keyPrefix) == namePrefix.length() + nameSize) ) {
				key= path.substr(namePrefix.length() + nameSize + keyPrefix.length(), keySize);
				try {
					hash::sha256	asHash= hash::sha256::fromHex(key);
					crypto::AES256(asHash.buffer(), asHash.size());
				} catch(const msg::Exception &) {
					return false;
				}
				if (path.length() > namePrefix.length() + nameSize + keyPrefix.length() + keySize) {
					suffix= path.substr(namePrefix.length() + nameSize + keyPrefix.length() + keySize);
					return '/' == path[namePrefix.length() + nameSize + keyPrefix.length() + keySize];
				} else {
					return true;
				}
			}
			return path.length() == namePrefix.length() + nameSize;
		}
		return false;
	}
	inline std::string HTTPHandler::_getData(const std::string &name, const std::string &key, const std::string &suffix, std::string &contentType) {
		std::string results;

		contentType.clear();
		if (_store.has(name)) {
			std::string			content= key.length() > 0 ? compute::unstash(_store, name, key) : _store.get(name);
			const std::string	expectedContentHash= key.length() > 0 ? key : name;

			if (hash::sha256(content).hex() != expectedContentHash) {
				content= z::uncompress(content, 2 * 1024 * 1024);
			}
			if (hash::sha256(content).hex() == expectedContentHash) {
				if (key.size() > 0) {
					json::Value	directory(content);
					std::string	path = suffix.substr(1);
					std::string subname, subkey, subsuffix;

					if ( (path.length() == 0) && (directory.has(path)) ) {
						path= directory[path].string();
					}
					if ( directory.has(path) && (directory[path].has("path"))
							&& (_isDataPath(directory[path]["path"], subname, subkey, subsuffix)) && (subsuffix.length() == 0) ) {
						results= _getData(subname, subkey, subsuffix, contentType);
						if (directory[path].has("Content-Type")) {
							contentType= directory[path]["Content-Type"];
						}
					}
				} else {
					results= content;
				}
			}
		}
		return results;
	}

	inline HTTP::HTTP(int port, store::Storage &storage):exec::Thread(KeepAroundAfterFinish),_store(storage),_port(port),_handlers() {
		start();
	}
	inline HTTP::~HTTP() {
		for (_HandlerList::iterator i= _handlers.begin(); i != _handlers.end(); ++i) {
			delete *i;
		}
	}
	inline void *HTTP::run() {
		net::AddressIPv4	address(_port);
		net::SocketServer	server(address.family());

		server.reuseAddress();
		server.reusePort();
		server.bind(address);
		server.listen(10/* backlog */);
		while (true) {
			net::AddressIPv6	connectedTo;
			net::Socket			*connection= new net::Socket();
			HTTPHandler			*found= NULL;

			server.accept(connectedTo, *connection);
			for (_HandlerList::iterator i= _handlers.begin(); i != _handlers.end(); ++i) {
				if ( (*i)->handleConnection(connection) ) {
					found= *i;
					break;
				}
			}
			if (NULL == found) {
				found= new HTTPHandler(_store);
				found->handleConnection(connection);
				_handlers.push_back(found);
			}
		}
	}

}


#endif // __Server_h__
