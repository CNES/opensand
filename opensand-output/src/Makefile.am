EXTRA_DIST = $(TESTS)

TESTS_ENVIRONMENT = top_builddir=$(top_builddir)

TESTS = run_output_tests.py

noinst_PROGRAMS = test_output
lib_LTLIBRARIES = libopensand_output.la

libopensand_output_la_cpp = \
	Messages.cpp \
	CommandThread.cpp \
	Output.cpp \
	BaseProbe.cpp \
	Event.cpp \
	OutputInternal.cpp \
	Probe.cpp

libopensand_output_la_h = \
	BaseProbe.h \
	CommandThread.h \
	Output.h \
	OutputInternal.h \
	Event.h \
	Messages.h \
	Probe.h

libopensand_output_la_SOURCES = \
	$(libopensand_output_la_cpp) \
	$(libopensand_output_la_h)

libopensand_output_la_LDFLAGS = -lpthread -lrt

test_output_SOURCES = test_output.cpp
test_output_LDADD = libopensand_output.la

libopensand_output_includedir = ${includedir}/opensand_output

libopensand_output_include_HEADERS = \
	BaseProbe.h \
	Output.h \
	OutputInternal.h \
	Event.h \
	Probe.h

