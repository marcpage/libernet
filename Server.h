#ifndef __Server_h__
#define __Server_h__

#include "os/Queue.h"
#include "os/Thread.h"
#include "os/SocketServer.h"
#include "os/AddressIPv4.h"
#include "os/AddressIPv6.h"
#include "os/BufferManaged.h"
#include "os/BufferString.h"
#include "libernet/Storage.h"
#include <vector>

namespace server {

	class HTTPHandler : public exec::Thread {
		public:
			HTTPHandler(store::Storage &store);
			virtual ~HTTPHandler();
			bool handle(net::Socket *connection);
		protected:
			virtual void *run();
		private:
			store::Storage				&_store;
			bool						_working;
			exec::Queue<net::Socket*>	_queue;

	};

	class HTTP : public exec::Thread {
		public:
			HTTP(int port, store::Storage &storage);
			virtual ~HTTP();
		protected:
			virtual void *run();
		private:
			store::Storage				&_store;
			int							_port;
			std::vector<HTTPHandler>	_handlers;

	};

	inline HTTPHandler::HTTPHandler(store::Storage &store):exec::Thread(KeepAroundAfterFinish),_store(store),_working(false),_queue() {
		start();
	}
	inline HTTPHandler::~HTTPHandler() {
	}
	inline bool HTTPHandler::handle(net::Socket *connection) {
	}
	inline void *HTTPHandler::run() {
	}

	inline HTTP::HTTP(int port, store::Storage &storage):exec::Thread(KeepAroundAfterFinish),_store(store),_port(port),_handlers() {
	}
	inline HTTP::~HTTP() {
	}
	inline void *HTTP::run() {
	}

}


#endif // __Server_h__
