.PHONY:clean venv test coverage debug lint serve format
all:clean test coverage lint

INITIAL_PYTHON?=python3
VENV_DIR?=.venv
VENV_PYTHON?=$(VENV_DIR)/bin/$(INITIAL_PYTHON)
VENV_PIP?=$(VENV_DIR)/bin/pip3
SET_ENV?=. $(VENV_DIR)/bin/activate

$(VENV_DIR)/touchfile: requirements.txt
	@test -d $(VENV_DIR) || $(INITIAL_PYTHON) -m venv $(VENV_DIR)
	@echo Ensuring pip is latest version
	@$(SET_ENV); $(VENV_PIP) install --quiet --upgrade pip
	@echo Fetching requirements
	@$(SET_ENV); $(VENV_PIP) install --quiet --upgrade --requirement $^
	@touch $@

venv: $(VENV_DIR)/touchfile

test: venv
	@$(SET_ENV); $(VENV_PYTHON) -m coverage run --source libernet -m pytest

coverage: test
	$(SET_ENV); $(VENV_PYTHON) -m coverage report -m --sort=cover --skip-covered

debug: venv
	$(SET_ENV); $(VENV_PYTHON) -m libernet.server --debug

serve: venv
	$(SET_ENV); $(VENV_PYTHON) -m libernet.server

format: venv
	$(SET_ENV); $(VENV_PYTHON) -m black libernet

lint: venv
	$(SET_ENV); $(VENV_PYTHON) -m pylint libernet
	$(SET_ENV); $(VENV_PYTHON) -m black libernet --check

clean:
	@rm -Rf $(VENV_DIR)
	@rm -f .coverage
	@rm -Rf .pytest_cache
	@find . -iname "*.pyc" -delete
