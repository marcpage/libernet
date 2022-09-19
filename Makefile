.PHONY:clean venv test coverage debug lint serve format
all:clean test coverage lint

INITIAL_PYTHON?=python3
VENV_DIR?=.venv
VENV_PYTHON?=$(VENV_DIR)/bin/$(INITIAL_PYTHON)
VENV_PIP?=$(VENV_DIR)/bin/pip3
SET_ENV?=. $(VENV_DIR)/bin/activate
SOURCES=$(shell find libernet -type f -iname "*.py")
TESTS=$(shell find tests -type f -iname "test_*.py")
COVERAGE_FILE=.coverage
FORMAT_FILE=$(VENV_DIR)/format.txt
LINT_FILE=$(VENV_DIR)/lint.txt

$(VENV_DIR)/touchfile: requirements.txt
	@test -d $(VENV_DIR) || $(INITIAL_PYTHON) -m venv $(VENV_DIR)
	@echo Ensuring pip is latest version
	@$(SET_ENV); $(VENV_PIP) install --quiet --upgrade pip
	@echo Fetching requirements
	@$(SET_ENV); $(VENV_PIP) install --quiet --upgrade --requirement $^
	@touch $@

venv: $(VENV_DIR)/touchfile

$(COVERAGE_FILE): $(VENV_DIR)/touchfile $(SOURCES) $(TESTS)
	@$(SET_ENV); $(VENV_PYTHON) -m coverage run --source libernet -m pytest

test: $(COVERAGE_FILE)

coverage: .coverage
	@$(SET_ENV); $(VENV_PYTHON) -m coverage report -m --sort=cover --skip-covered

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
	@rm -f .coverage
	@rm -Rf .pytest_cache
	@find . -iname "*.pyc" -delete
