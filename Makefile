.PHONY:venv test coverage debug run

all:venv test coverage debug run

venv:
	@python3 -m venv /tmp/libernet
	@. /tmp/libernet/bin/activate
	@pip3 install -q --upgrade pip
	@pip3 install -q -r requirements.txt

test: venv
	@python3 -m coverage run --source libernet -m pytest

coverage: test
	@python3 -m coverage report -m

debug: venv
	@python3 -m libernet.server --debug

run: venv
	@python3 -m libernet.server
