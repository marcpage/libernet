#include <stdio.h>
#include "libernet/Storage.h"
#include "os/Path.h"

#define dotest(condition) \
	if(!(condition)) { \
		fprintf(stderr, "FAIL(%s:%d): %s\n",__FILE__, __LINE__, #condition); \
	}

int main(const int /*argc*/, const char * const /*argv*/[]) {
	int	iterations= 100;
#ifdef __Tracer_h__
	iterations= 1;
#endif
	for(int i= 0; i < iterations; ++i) {
		try {
			io::Path where("bin/logs/storage");
			{
				store::Container container(where);
				container.put("john", "doe");
				printf("'%s' == 'doe'\n", container.get("john").c_str());
				dotest(container.get("john") == "doe");
			}
			{
				store::Container container(where);
				printf("'%s' == 'doe'\n", container.get("john").c_str());
				dotest(container.get("john") == "doe");
			}
			{
				store::Container container(where);
				container.put("john", "doer");
				printf("'%s' == 'doe'\n", container.get("john").c_str());
				dotest(container.get("john") == "doe");
			}
			{
				store::Container container(where);
				dotest(container.del("john"));
				printf("'%s' == ''\n", container.get("john").c_str());
				dotest(container.get("john") == "");
				dotest(!container.del("john"));
			}
			{
				store::Container container(where);
				container.put("john", "doer");
				printf("'%s' == 'doer'\n", container.get("john").c_str());
				dotest(container.get("john") == "doer");
			}
			where.remove();
		} catch(const std::exception &exception) {
			printf("FAIL: Exception: %s\n", exception.what());
		}
	}
	return 0;
}
