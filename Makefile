.PHONY:test docs

all:test docs

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
