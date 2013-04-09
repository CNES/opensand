lib_LTLIBRARIES = libopensand_rt.la

libopensand_rt_la_cpp = \
Block.cpp \
Channel.cpp \
BlockMgr.cpp \
Event.cpp \
MsgEvent.cpp \
TimerEvent.cpp \
NetSocketEvent.cpp  \
SignalEvent.cpp


libopensand_rt_la_h = \
Channel.h \
Types.h \
Block.h \
BlockMgr.h \
Event.h \
MsgEvent.h \
TimerEvent.h \
NetSocketEvent.h \
SignalEvent.h

libopensand_rt_la_SOURCES = $(libopensand_rt_la_cpp) $(libopensand_rt_la_h)

libopensand_rt_la_LIBADD =  /usr/lib/libtcmalloc_minimal.so -lrt -lpthread

libopensand_rt_includedir = ${includedir}/opensand_rt

libopensand_rt_include_HEADERS = \
Channel.h \
Types.h \
Block.h \
BlockMgr.h \
Event.h \
MsgEvent.h \
TimerEvent.h \
NetSocketEvent.h \
SignalEvent.h

EXTRA_DIST=readme.txt
