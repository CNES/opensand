EXTRA_DIST = $(TESTS)

TESTS_ENVIRONMENT = top_builddir=$(top_builddir)

TESTS = run_env_plane_tests.py

bin_PROGRAMS = 
noinst_PROGRAMS = test_env_plane
lib_LTLIBRARIES = libopensand_env_plane.la

libopensand_env_plane_la_cpp = \
messages.cpp \
CommandThread.cpp \
EnvPlane.cpp \
BaseProbe.cpp \
Event.cpp \
EnvPlaneInternal.cpp \
Probe.cpp

libopensand_env_plane_la_h = \
BaseProbe.h \
CommandThread.h \
EnvPlane.h \
EnvPlaneInternal.h \
Event.h \
messages.h \
Probe.h

libopensand_env_plane_la_SOURCES = $(libopensand_env_plane_la_cpp) $(libopensand_env_plane_la_h)
libopensand_env_plane_la_CPPFLAGS = -pthread -W -Wall -ansi -pedantic
libopensand_env_plane_la_LIBADD = -lrt
libopensand_env_plane_la_LDFLAGS = -pthread

test_env_plane_SOURCES = test_env_plane.cpp
test_env_plane_CPPFLAGS = -W -Wall -ansi -pedantic
test_env_plane_LDADD = libopensand_env_plane.la

libopensand_env_plane_includedir = ${includedir}/opensand_env_plane

libopensand_env_plane_include_HEADERS = \
BaseProbe.h \
EnvPlane.h \
EnvPlaneInternal.h \
Event.h \
Probe.h

