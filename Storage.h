#ifndef __Storage_h__
#define __Storage_h__

/** @file Storage.h
	@todo document
*/

#include "os/Path.h"
#include "os/Thread.h"
#include "os/Queue.h"
#include "libernet/Container.h"
#include <vector>

namespace store {

	class Storage : public exec::Thread {
		public:
			typedef std::string String;
			typedef std::vector<String> NameList;
			Storage(const io::Path &location);
			virtual ~Storage();
			void put(const String &name, const String &data);
			String &get(const String &name, String &buffer);
			String get(const String &name) {String buffer; return get(name, buffer);}
			void del(const String &name);
			bool has(const String &name);
			NameList &find(const String &like, int count, NameList &list);
			NameList find(const String &like, int count) {NameList list; return find(like, count, list);}
		protected:
			virtual void *run();
		private:
			struct Request {
				typedef exec::Queue<String> Queue;
				enum Action {Put, Get, Del, Has, Find};
				Request(const String &aName):action(Del),name(aName),data(),count(0),results(NULL) {}
				Request(const String &aName, const String &aData):action(Put),name(aName),data(aData),count(0),results(NULL) {}
				Request(const String &aName, int aCount, Queue &queue):action(Find),name(aName),data(),count(aCount),results(&queue) {}
				Request(Action anAction, const String &aName, Queue &queue):action(anAction),name(aName),data(),count(0),results(&queue) {}
				const char *c_str() const {return name.c_str();}
				Action action;
				String name;
				String data;
				int count;
				Queue *results;
			};
			store::Container		_container;
			exec::Queue<Request>	_requests;
	};

	inline Storage::Storage(const io::Path &location):_container(location),_requests() {
		start();
	}
	inline Storage::~Storage() {
	}
	inline void Storage::put(const String &name, const String &data) {
		_requests.enqueue(Request(name, data));
	}
	inline Storage::String &Storage::get(const String &name, String &buffer) {
		Request::Queue response;

		_requests.enqueue(Request(Request::Get, name, response));
		buffer= response.dequeue();
		return buffer;
	}
	inline void Storage::del(const String &name) {
		_requests.enqueue(Request(name));
	}
	inline bool Storage::has(const String &name) {
		Request::Queue response;
		_requests.enqueue(Request(Request::Has, name, response));
		return response.dequeue().length() > 0;
	}
	inline Storage::NameList &Storage::find(const String &like, int count, NameList &list) {
		Request::Queue	response;
		String			name;

		_requests.enqueue(Request(like, count, response));
		list.clear();
		while ( (name= response.dequeue()).length() > 0 ) {
			list.push_back(name);
		}
		return list;
	}
	inline void *Storage::run() {
		Container::NameList names;

		while (true) {
			Request next= _requests.dequeue();

			switch(next.action) {
				case Request::Put:
					_container.put(next.name, next.data);
					break;
				case Request::Get:
					next.results->enqueue(_container.get(next.name, next.data));
					break;
				case Request::Del:
					_container.del(next.name);
					break;
				case Request::Has:
					next.results->enqueue(_container.has(next.name) ? "1" : "");
					break;
				case Request::Find:
					_container.find(next.name, next.count, names);
					for (Container::NameList::iterator name= names.begin(); name != names.end(); ++name) {
						next.results->enqueue(name->first);
					}
					next.results->enqueue("");
					break;
			}
		}
	}

}

#endif // __Container_h__
