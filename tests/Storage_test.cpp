#include <stdio.h>
#include "libernet/Storage.h"
#include "os/Path.h"
#include "os/Thread.h"

#ifdef __Tracer_h__
const int RunCount= 20;
#else
const int RunCount= 1000;
#endif

#define dotest(condition) \
	if(!(condition)) { \
		fprintf(stderr, "FAIL(%s:%d): %s\n",__FILE__, __LINE__, #condition); \
	}

class Pounder : public exec::Thread {
	public:
		Pounder(const std::string &name, store::Storage store):_name(name),_store(store) {}
		virtual ~Pounder() {}
	protected:
		virtual void *run() {
			const int halfCount= RunCount / 2;
			store::Storage::NameList names;

			for (int i= 0; i < RunCount; ++i) {
				_store.put(name + "_" + std::to_string(i),name + "_" + std::to_string(i) + "_data");
			}
			for (int i= 0; i < RunCount; ++i) {
				dotest(_store.has(name + "_" + std::to_string(i)));
			}
			for (int i= 0; i < RunCount; ++i) {
				dotest(_store.get(name + "_" + std::to_string(i),name + "_" + std::to_string(i) + "_data") == name + "_" + std::to_string(i),name + "_" + std::to_string(i) + "_data");
			}
			for (int i= 0; i < halfCount; ++i) {
				dotest(_store.del(name + "_" + std::to_string(i)));
			}
			for (int i= 0; i < halfCount; ++i) {
				dotest(!_store.has(name + "_" + std::to_string(i)));
			}
			for (int i= halfCount; i < RunCount; ++i) {
				dotest(_store.has(name + "_" + std::to_string(i)));
			}
			names= _store.find(name, RunCount);
			dotest(names.length() == (RunCount - halfCount));
		}
	private:
		std::string		_name;
		store::Storage	&_store;
};

int main(const int /*argc*/, const char * const /*argv*/[]) {
	try {
	} catch(const std::exception &exception) {
		printf("FAIL: Exception: %s\n", exception.what());
	}
	return 0;
}
