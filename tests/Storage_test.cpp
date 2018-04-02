#include <stdio.h>
#include "libernet/Storage.h"
#include "os/Path.h"
#include "os/Thread.h"

#ifdef __Tracer_h__
const int RunCount= 4;
#else
const int RunCount= 45;
#endif

#define dotest(condition) \
	if(!(condition)) { \
		fprintf(stderr, "FAIL(%s:%d): %s\n",__FILE__, __LINE__, #condition); \
	}

class Pounder : public exec::Thread {
	public:
		Pounder(const std::string &name, store::Storage &store):exec::Thread(KeepAroundAfterFinish),_name(name),_store(store) {start();}
		virtual ~Pounder() {}
	protected:
		virtual void *run() {
			const int halfCount= RunCount / 2;
			store::Storage::NameList names;

			printf("Put %d %s\n", RunCount, _name.c_str());
			for (int i= 0; i < RunCount; ++i) {
				_store.put(_name + "_" + std::to_string(i),_name + "_" + std::to_string(i) + "_data");
			}
			printf("Has %d %s\n", RunCount, _name.c_str());
			for (int i= 0; i < RunCount; ++i) {
				dotest(_store.has(_name + "_" + std::to_string(i)));
			}
			printf("Get %d %s\n", RunCount, _name.c_str());
			for (int i= 0; i < RunCount; ++i) {
				dotest(_store.get(_name + "_" + std::to_string(i)) == _name + "_" + std::to_string(i) + "_data");
			}
			printf("Del %d %s\n", RunCount, _name.c_str());
			for (int i= 0; i < halfCount; ++i) {
				_store.del(_name + "_" + std::to_string(i));
			}
			printf("!Has %d %s\n", RunCount, _name.c_str());
			for (int i= 0; i < halfCount; ++i) {
				dotest(!_store.has(_name + "_" + std::to_string(i)));
			}
			printf("Has %d %s\n", RunCount, _name.c_str());
			for (int i= halfCount; i < RunCount; ++i) {
				dotest(_store.has(_name + "_" + std::to_string(i)));
			}
			names= _store.find(_name, RunCount);
			printf("names.size() == %li RunCount = %d halfCount = %d\n", names.size(), RunCount, halfCount);
			dotest(names.size() == RunCount);
			return NULL;
		}
	private:
		std::string		_name;
		store::Storage	&_store;
};

int main(const int /*argc*/, const char * const /*argv*/[]) {
	try {
		io::Path where("bin/scratch/Storage");

		if (where.isDirectory()) {
			where.remove();
		}

		store::Storage storage(where);
		Pounder p1("one",storage), p2("two",storage), p3("three",storage), p4("four",storage);
		Pounder	p5("five",storage), p6("six",storage), p7("seven",storage), p8("eight",storage);
		Pounder p9("nine",storage), p10("ten",storage);
		Pounder *threads[]= {&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10};

		for(size_t i= 0; i < sizeof(threads)/sizeof(threads[0]); ++i) {
			threads[i]->join();
		}

	} catch(const std::exception &exception) {
		printf("FAIL: Exception: %s\n", exception.what());
	}
	return 0;
}
