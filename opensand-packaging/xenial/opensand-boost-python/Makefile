ARCH := $(shell dpkg --print-architecture)
ifeq ($(ARCH), amd64)
	LIBDIR=lib/x86_64-linux-gnu/
else ifeq ($(ARCH), i386)
	LIBDIR=lib/i386-linux-gnu/
else
	LIBDIR=lib/
endif


all: build

build: src/boost_1_75_0.tar.bz2
	cd src; tar -xvjf boost_1_75_0.tar.bz2
	cd src/boost_1_75_0; CC=gcc-7 CXX=g++-7 ./bootstrap.sh --with-toolset=gcc --with-python=/usr/bin/python3.5 --prefix=/usr
	cd src/boost_1_75_0; ./b2 toolset=gcc-7 --with-python --prefix=../usr/ --libdir=../usr/${LIBDIR} install

src/boost_1_75_0.tar.bz2:
	mkdir -p src
	cd src; wget https://boostorg.jfrog.io/artifactory/main/release/1.75.0/source/boost_1_75_0.tar.bz2

clean:
	rm -rf src/boost_1_75_0/
	rm -rf src/usr/

.PHONY: all build clean
