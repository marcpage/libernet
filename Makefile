.PHONY:clean venv test coverage debug lint serve format
all:clean test coverage lint

INITIAL_PYTHON?=python3
VENV_DIR?=.venv
VENV_PYTHON?=$(VENV_DIR)/bin/$(INITIAL_PYTHON)
VENV_PIP?=$(VENV_DIR)/bin/pip3
SET_ENV?=. $(VENV_DIR)/bin/activate
SOURCES=$(shell find libernet -type f -iname "*.py")
TESTS=$(shell find tests -type f -iname "test_*.py")
COVERAGE_FILE=objects/coverage.bin  # must match data-file in coverage.rc
FORMAT_FILE=$(VENV_DIR)/format.txt
LINT_FILE=$(VENV_DIR)/lint.txt
COVERAGE=COVERAGE_PROCESS_START=coverage.rc

$(VENV_DIR)/touchfile: requirements.txt
	@test -d $(VENV_DIR) || $(INITIAL_PYTHON) -m venv $(VENV_DIR)
	@echo Ensuring pip is latest version
	@$(SET_ENV); $(VENV_PIP) install --quiet --upgrade pip
	@echo Fetching requirements
	@$(SET_ENV); $(VENV_PIP) install --quiet --upgrade --requirement $^
	@touch $@

venv: $(VENV_DIR)/touchfile

# https://stackoverflow.com/questions/28297497/python-code-coverage-and-multiprocessing
$(COVERAGE_FILE): $(VENV_DIR)/touchfile $(SOURCES) $(TESTS)
	mkdir -p objects
	@$(SET_ENV); env $(COVERAGE) $(VENV_PYTHON) -m coverage run -m pytest
	@$(SET_ENV); $(VENV_PYTHON) -m coverage combine --data-file=$(COVERAGE_FILE)

test: $(COVERAGE_FILE)

coverage: $(COVERAGE_FILE)
	@$(SET_ENV); $(VENV_PYTHON) -m coverage report -m --sort=cover --skip-covered --data-file=$(COVERAGE_FILE)

debug: venv
	$(SET_ENV); $(VENV_PYTHON) -m libernet.server --debug

serve: venv
	$(SET_ENV); $(VENV_PYTHON) -m libernet.server

$(FORMAT_FILE): $(VENV_DIR)/touchfile $(SOURCES)
	@$(SET_ENV); $(VENV_PYTHON) -m black libernet &> $@

format: $(FORMAT_FILE)
	@cat $^

$(LINT_FILE): $(VENV_DIR)/touchfile $(SOURCES)
	@$(SET_ENV); $(VENV_PYTHON) -m pylint libernet &> $@
	@$(SET_ENV); $(VENV_PYTHON) -m black libernet --check >> $@  2>&1

lint: $(LINT_FILE)
	@cat $^

clean:
	@rm -Rf $(VENV_DIR)
	@rm -Rf objects
	@rm -f $(strip $(COVERAGE_FILE))*
	@rm -Rf .pytest_cache
	@find . -iname "*.pyc" -delete
