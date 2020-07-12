all: 00 01 02 03 04 05 06 07 08

PYTHON=python3
%: str%.py
	time $(PYTHON) $<

CFLAGS=$(shell $(PYTHON)-config --includes)- -fPIC -O3 -DNDEBUG -fwrapv -g
CC=clang
%.so: %.o
	clang $< -shared -o $@

%: str%.so
	time $(PYTHON) -c "import $(patsubst %.so,%,$<)"

js:
	time node str.js

pypy:
	time pypy3 str01.py
