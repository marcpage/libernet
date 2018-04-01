#ifndef __Server_h__
#define __Server_h__

#include "os/Queue.h"
#include "os/Thread.h"
#include "os/SocketServer.h"
#include "os/AddressIPv4.h"
#include "os/AddressIPv6.h"
#include "os/BufferManaged.h"
#include "os/BufferString.h"
#include <vector>

namespace server {

	class HTTPHandler : public exec::Thrad {
		public:
			HTTPHandler();
			~HTTPHandler();
			bool handle(net::Socket *connection);
		protected:
			virtual void *run();
		private:
			bool						_working;
			exec::Queue<net::Socket*>	_queue;

	};

	class HTTP : public exec::Thread {
		public:
			HTTP(int port);
			virtual ~HTTP();
		protected:
			virtual void *run();
		private:
			int							_port;
			std::vector<HTTPHandler>	_handlers;

	};

}


#endif // __Server_h__
