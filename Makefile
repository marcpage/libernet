.PHONY:venv test coverage debug lint serve format

all:venv test coverage lint

venv:
	@python3 -m venv .venv
	@. .venv/bin/activate
	@pip3 install -q --upgrade pip
	@pip3 install -q -r requirements.txt

test: venv
	@python3 -m coverage run --source libernet -m pytest

coverage: test
	@python3 -m coverage report -m

debug: venv
	@python3 -m libernet.server --debug

serve: venv
	@python3 -m libernet.server

format: venv
	@python3 -m black libernet

lint: venv
	@python3 -m pylint libernet
	@python3 -m black libernet --check
