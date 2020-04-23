.PHONY:test docs

all:test docs lint

lint:bin/logs/lint.txt

OPENSSL_PATH=$(subst openssl=,-I,$(OS_OPTIONS))/include

KNOWN_ERRORS:= --suppress=unusedFunction \
    			--inline-suppr \
				-U_DEBUG_FILE

check:format lint docs

bin/logs/lint.txt: *.h
	@echo Linting ...
	@mkdir -p bin/logs
	@cppcheck --enable=all --force --std=c++11 $(KNOWN_ERRORS) --language=c++ $(OPENSSL_PATH) -I.. *.h &> $@
	@-cat $@ | grep performance: || true
	@-cat $@ | grep portability: || true
	@-cat $@ | grep style: || true
	@-cat $@ | grep warning: || true
	@-cat $@ | grep error: || true
	@grep -rniw todo *.h
	@echo `grep -rniw todo *.h | wc -l` TODO items

documentation/index.html:
	@mkdir -p documentation
	@doxygen libernet.dox 2> bin/logs/doxygen.txt
	@if [ `cat bin/logs/doxygen.txt | wc -l` -ne "0" ]; then echo `cat bin/logs/doxygen.txt | wc -l` documentation messages; fi

docs:documentation/index.html

test:bin/test
	@bin/test $(OS_OPTIONS) $(COMPILER) $(TEST)

bin/test:format
test:format
docs:format
lint:format

format:bin/logs/clang-format.txt

bin/logs/clang-format.txt:tests/*.cpp *.h
	@echo Cleaning code ...
	@mkdir -p bin/logs/
	@clang-format --verbose -i *.h tests/*.cpp 2> bin/logs/clang-format.txt

../os/tests/test.cpp:
	@git clone http://github.com/marcpage/os ../os

../os/*.h:../os/tests/test.cpp

bin/test:../os/tests/test.cpp ../os/*.h *.h
	@mkdir -p bin
	@clang++ ../os/tests/test.cpp -o $@ -I.. -std=c++11 -lsqlite3 -Wall -Weffc++ -Wextra -Wshadow -Wwrite-strings

bin/%:%.cpp
	@clang++ $< -o -o $@ -std-c++11 -I.. -std=c++11 -lsqlite3 -Wall -Weffc++ -Wextra -Wshadow -Wwrite-strings

clean:
	@rm -Rf documentation bin/coverage bin/test bin/tests bin/logs/*.log bin/logs/*.txt
