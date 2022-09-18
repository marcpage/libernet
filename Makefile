.PHONY:clean venv test coverage debug lint serve format
all:clean venv test coverage lint

INITIAL_PYTHON?=python3
VENV_DIR?=.venv
VENV_PYTHON?=$(VENV_DIR)/bin/$(INITIAL_PYTHON)

venv:
	@$(INITIAL_PYTHON) -m venv $(VENV_DIR)
	@. $(VENV_DIR)/bin/activate
	@pip3 install -q --upgrade pip
	@pip3 install -q -r requirements.txt

test: venv
	@$(VENV_PYTHON) -m coverage run --source libernet -m pytest

coverage: test
	@$(VENV_PYTHON) -m coverage report -m --sort=cover --skip-covered

debug: venv
	@$(VENV_PYTHON) -m libernet.server --debug

serve: venv
	@$(VENV_PYTHON) -m libernet.server

format: venv
	@$(VENV_PYTHON) -m black libernet

lint: venv
	@$(VENV_PYTHON) -m pylint libernet
	@$(VENV_PYTHON) -m black libernet --check

clean:
	@rm -Rf $(VENV_DIR)
	@rm -f .coverage
	@rm -Rf .pytest_cache


