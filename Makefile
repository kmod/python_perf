all: build/cpython_install/bin/python build/cpython_install_pgo/bin/python

build build/cpython_install build/cpython_install_pgo build/cpython_build build/cpython_build_pgo:
	mkdir -p $@

build/cpython_build/Makefile: cpython/configure | build/cpython_build
	cd build/cpython_build; ../../cpython/configure --prefix=$(CURDIR)/build/cpython_install

build/cpython_build_pgo/Makefile: cpython/configure | build/cpython_build_pgo
	cd build/cpython_build_pgo; ../../cpython/configure --prefix=$(CURDIR)/build/cpython_install_pgo --enable-optimizations

build/cpython_build/python: build/cpython_build/Makefile
	$(MAKE) -C build/cpython_build

build/cpython_build_pgo/python: build/cpython_build_pgo/Makefile
	$(MAKE) -C build/cpython_build_pgo

build/cpython_install/bin/python3: build/cpython_build/python
	$(MAKE) -C build/cpython_build install

build/cpython_install_pgo/bin/python3: build/cpython_build_pgo/python
	$(MAKE) -C build/cpython_build_pgo install

build/env/bin/python: build/cpython_install/bin/python3
	rm -rfv build/env
	(virtualenv -p $< build/env; $@/bin/pip install -r requirements.txt) || rm -rfv build/env

build/pgo_env/bin/python: build/cpython_install_pgo/bin/python3
	rm -rfv build/pgo_env
	(virtualenv -p $< build/pgo_env; $@/bin/pip install -r requirements.txt) || rm -rfv build/pgo_env

.PHONY: pyperformance pyperformance_pgo
pyperformance: build/env/bin/python
	# PYTHONPATH=pyperformance build/env/bin/python pyperformance/runtests.py
	$< -m pyperformance run -o pyperformance.json

pyperformance.json: build/env/bin/python
	$< -m pyperformance run -o pyperformance.json

pyperformance_pgo: build/pgo_env/bin/python
	$< -m pyperformance run -o pyperformance_pgo.json

pyperformance_pgo.json: build/pgo_env/bin/python
	$< -m pyperformance run -o pyperformance_pgo.json

