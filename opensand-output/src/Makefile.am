EXTRA_DIST = $(TESTS)

TESTS_ENVIRONMENT = top_builddir=$(top_builddir)

TESTS = run_output_tests.py

noinst_PROGRAMS = test_output
lib_LTLIBRARIES = libopensand_output.la

libopensand_output_la_cpp = \
	BaseProbe.cpp \
	Output.cpp \
	OutputEvent.cpp \
	OutputLog.cpp \
	OutputHandler.cpp \
	Probe.cpp

libopensand_output_la_h = \
	BaseProbe.h \
	Output.h \
	OutputEvent.h \
	OutputLog.h \
	OutputHandler.h \
	OutputMutex.h \
	Printf.h \
	Probe.h

libopensand_output_la_SOURCES = \
	$(libopensand_output_la_cpp) \
	$(libopensand_output_la_h)

libopensand_output_la_LDFLAGS = -lpthread -lrt -ldl -lstdc++fs

test_output_SOURCES = test_output.cpp
test_output_LDADD = libopensand_output.la

libopensand_output_includedir = ${includedir}/opensand_output

libopensand_output_include_HEADERS = \
	$(libopensand_output_la_h)
