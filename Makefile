.PHONY:test docs

all:test docs lint

test:runtests coverage
lint:todo

OPENSSL_PATH=$(subst openssl=,-I,$(OS_OPTIONS))/include

PLATFORM = $(shell uname)
CODE_FORMAT_TOOL = $(shell which clang-format)
DOCUMENTATION_TOOL = $(shell which doxygen)
LINT_TOOL = $(shell which cppcheck)

ifeq ($(PLATFORM),Darwin)
  CLANG_FORMAT_FLAGS = --verbose
  SANITIZERS = -fsanitize=address -fsanitize-address-use-after-scope -fsanitize=undefined
endif

ifeq ($(PLATFORM),Linux)
  USE_OPENSSL = -DOpenSSLAvailable=1 -lcrypto
endif

KNOWN_ERRORS:= --suppress=unusedFunction \
    			--inline-suppr \
				-U_DEBUG_FILE

check:format lint docs

bin/logs/lint.txt: src/*/*.h
	@echo Linting ...
	@mkdir -p bin/logs
	@cppcheck --enable=all --force --std=c++11 $(KNOWN_ERRORS) --language=c++ $(OPENSSL_PATH) -Isrc src/*/*.h &> $@
	@-cat $@ | grep performance: || true
	@-cat $@ | grep portability: || true
	@-cat $@ | grep style: || true
	@-cat $@ | grep warning: || true
	@-cat $@ | grep error: || true

todo:
	@grep -rniw todo src/*/*.h
	@echo `grep -rniw todo src/*/*.h | wc -l` TODO items

coverage:runtests
	@cat bin/coverage/*/*.gcov | grep -E '[0-9]+:' | grep -ve -: | grep -v "#####" | grep -v "=====" > bin/logs/all_code_coverage.txt
	@grep // bin/logs/all_code_coverage.txt | grep -i test | grep -ivw $(PLATFORM)| grep -vw libernet | sort | uniq  || true
	@echo `grep // bin/logs/all_code_coverage.txt | grep -i test | grep -ivw $(PLATFORM)| grep -vw libernet | sort | uniq | wc -l` lines now tested

documentation/index.html:
	@mkdir -p documentation
	@$(DOCUMENTATION_TOOL) libernet.dox 2> bin/logs/doxygen.txt
	@if [ `cat bin/logs/doxygen.txt | wc -l` -ne "0" ]; then echo `cat bin/logs/doxygen.txt | wc -l` documentation messages; fi

ifneq (,$(strip $(DOCUMENTATION_TOOL)))
docs:documentation/index.html
else
docs:
	@$(ECHO) doxygen tool not found, no documentation will be generated
endif

ifneq (,$(strip $(LINT_TOOL)))
lint:bin/logs/lint.txt
else
lint:
	@$(ECHO) cppcheck tool not found, no linting will be performed
endif

performance:bin/test
	@bin/test $(OS_OPTIONS) $(COMPILER) $(TEST)

runtests:bin/test
	@bin/test $(OS_OPTIONS) $(COMPILER) test $(TEST)

bin/test:format
test:format
docs:format
lint:format

ifneq (,$(strip $(CODE_FORMAT_TOOL)))
format:bin/logs/clang-format.txt
else
format:
	@$(ECHO) clang-format tool not found, no code formatting will be performed
endif

bin/logs/clang-format.txt:tests/*.cpp src/*/*.h
	@echo Cleaning code ...
	@mkdir -p bin/logs/
	@$(CODE_FORMAT_TOOL) $(CLANG_FORMAT_FLAGS) -i src/*/*.h tests/*.cpp 2> bin/logs/clang-format.txt

# -fsanitize=memory
# -fsanitize=thread
# -flto -fsanitize=cfi
# -fsanitize=leak
# -fsanitize=safe-stack
# -D_LIBCPP_DEBUG=1
bin/test:tests/test.cpp src/*/*.h
	@mkdir -p bin
	@clang++ tests/test.cpp -o $@ $(USE_OPENSSL) -Isrc -std=c++11 -lsqlite3 -Wall -Weffc++ -Wextra -Wshadow -Wwrite-strings $(SANITIZERS) -fno-optimize-sibling-calls -O0 -g

bin/%:%.cpp
	@clang++ $< -o -o $@ $(USE_OPENSSL) -std-c++11 -Isrc -std=c++11 -lsqlite3 -Wall -Weffc++ -Wextra -Wshadow -Wwrite-strings $(SANITIZERS) -fno-optimize-sibling-calls -O0 -g

clean:
	@rm -Rf documentation bin/coverage bin/test bin/tests bin/logs/*.log bin/logs/*.txt

# bin/coverage/DateTime_clang++_trace/DateTime.h.gcov| sed -E 's/^([^:]+:)([^:]+:)/\2\1/' | sort | uniq
