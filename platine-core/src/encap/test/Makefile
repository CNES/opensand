CC=g++
CPPFLAGS=-I.. -I../../common -I../../../cq_runtime/include -g
LD=$(CC)
LDFLAGS=-L../.libs -L../../common/.libs -L../../../cq_runtime/lib
LDADD=-lsatsix_encap -lsatsix_common -lmgl_runtime_lib_static -lpthread -lpcap

.PHONY: test.o test

all: test

test.o: test.cpp
	$(CC) -c $(CPPFLAGS) test.cpp
test: test.o
	$(LD) -o test $(LDFLAGS) test.o $(LDADD)

clean:
	$(RM) *.o test

report:
	./report.sh

