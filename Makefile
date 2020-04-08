.PHONY:test docs

all:test docs lint

lint:bin/logs/lint.txt

OPENSSL_PATH=$(subst openssl=,-I,$(OS_OPTIONS))/include

bin/logs/lint.txt: *.h
	@mkdir -p bin/logs
	@cppcheck --enable=all --force --std=c++11 --suppress=unusedFunction --suppress=constParameter --language=c++ $(OPENSSL_PATH) -I/usr/include -I.. *.h &> $@
	@-cat $@ | grep performance:
	@-cat $@ | grep portability:
	@-cat $@ | grep style:
	@-cat $@ | grep warning:
	@-cat $@ | grep error:

documentation/index.html:
	@mkdir -p documentation
	@doxygen libernet.dox 2> bin/logs/doxygen.txt
	@if [ `cat bin/logs/doxygen.txt | wc -l` -ne "0" ]; then echo `cat bin/logs/doxygen.txt | wc -l` documentation messages; fi

docs:documentation/index.html

test:bin/test
	@bin/test $(OS_OPTIONS) $(COMPILER) $(TEST)

../os/tests/test.cpp:
	@git clone http://github.com/marcpage/os ../os

../os/*.h:../os/tests/test.cpp

bin/test:../os/tests/test.cpp ../os/*.h *.h
	@mkdir -p bin
	@clang++ ../os/tests/test.cpp -o $@ -std=c++11 -I.. -lsqlite3 -Wall -Weffc++ -Wextra -Wshadow -Wwrite-strings

bin/%:%.cpp
	@clang++ $< -o -o $@ -std-c++11 -I.. -lsqlite3 -Wall -Weffc++ -Wextra -Wshadow -Wwrite-strings

clean:
	@rm -Rf documentation bin/coverage bin/test bin/tests bin/logs/*.log bin/logs/*.txt
