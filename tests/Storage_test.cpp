#include <stdio.h>
#include "libernet/Storage.h"
#include "os/Path.h"

#define dotest(condition) \
	if(!(condition)) { \
		fprintf(stderr, "FAIL(%s:%d): %s\n",__FILE__, __LINE__, #condition); \
	}

int main(const int /*argc*/, const char * const /*argv*/[]) {
	int	iterations= 2;
#ifdef __Tracer_h__
	iterations= 1;
#endif
	for(int i= 0; i < iterations; ++i) {
		try {
			io::Path where("bin/logs/storage");

			if (where.isDirectory()) {
				where.remove();
			}
			try {
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
				{
					store::Container container(where);
					store::Container::NameList found;

					container.put("jack", "doelbig");
					container.put("jhonathan", "doeis");
					container.put("jhonnathan", "doll");
					container.put("johnathan", "dough");
					container.put("johnathen", "doeing");
					container.put("johnathon", "doclar");
					container.put("jon", "doel");
					container.put("jonatan", "doleby");
					container.put("jonathan", "doe");
					container.put("jonothan", "doford");
					container.put("jonothon", "dow");
					container.put("yehonasan", "doen");
					container.put("yehonathan", "dolsen");
					container.put("yeonoson", "doemich");
					container.put("yhonathan", "dorfeo");
					container.put("yonatan", "doelling");
					container.put("yonathan", "doella");
					container.put("yonaton", "doelner");
					container.put("yonoson", "doelter");
					found= container.find("johnathan", 4);
					dotest(found.size() == 4);
					dotest(found.find("john") != found.end());
					dotest(found.find("johnathan") != found.end());
					dotest(found.find("johnathen") != found.end());
					dotest(found.find("johnathon") != found.end());
					found= container.find("johnathan", 9);
					dotest(found.find("john") != found.end());
					dotest(found.find("johnathan") != found.end());
					dotest(found.find("johnathen") != found.end());
					dotest(found.find("johnathon") != found.end());
					dotest(found.find("jon") != found.end());
					dotest(found.find("jonatan") != found.end());
					dotest(found.find("jonathan") != found.end());
					dotest(found.find("jonothan") != found.end());
					dotest(found.find("jonothon") != found.end());
					printf("johnathan\n");
					for (store::Container::NameList::iterator name= found.begin(); name != found.end(); ++name) {
						printf("\t%s size=%ld have=%s\n", name->first.c_str(), name->second.size, name->second.have ? "true" : "false");
					}
				}
				{
					printf("Testing removable create\n");
					store::Container container(where);
					printf("Testing removable removable\n");
					store::Container::NameList found= container.removable("john", 5);;
					printf("Testing removable done\n");
				}
			} catch(const std::exception &exception) {
				printf("FAIL: Exception: %s\n", exception.what());
			}
		} catch(const std::exception &exception) {
			printf("FAIL: Exception: %s\n", exception.what());
		}
	}
	return 0;
}
