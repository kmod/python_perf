all: build/cpython_install/bin/python build/cpython_install_pgo/bin/python

build build/cpython_install build/cpython_install_pgo build/cpython_build build/cpython_build_pgo:
	mkdir -p $@

build/cpython_build/Makefile: | build/cpython_build
	cd build/cpython_build; ../../cpython/configure

build/cpython_build_pgo/Makefile: | build/cpython_build_pgo
	cd build/cpython_build_pgo; ../../cpython/configure --enable-optimizations

build/cpython_build/python: build/cpython_build/Makefile
	$(MAKE) -C build/cpython_build

build/cpython_build_pgo/python: build/cpython_build_pgo/Makefile
	$(MAKE) -C build/cpython_build_pgo

build/cpython_install/bin/python: build/cpython_build/python
	$(MAKE) -C build/cpython_build install

build/cpython_install_pgo/bin/python: build/cpython_build_pgo/python
	$(MAKE) -C build/cpython_build_pgo install
