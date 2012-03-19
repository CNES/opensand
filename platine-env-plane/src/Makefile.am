bin_PROGRAMS = ErrorController EventController ProbeController  
noinst_PROGRAMS = TestEnvironmentAgent
lib_LTLIBRARIES = libplatine_env_plane.la

libplatine_env_plane_la_c = \
Trace.c \
Error.c \
Time.c \
ComParameters.c \
IPAddr.c \
GenericPacket.c \
GenericPort.c \
UDPSocket.c \
SizedBuffer.c \
CircularBuffer.c\
LineParser.c \
FileInfos.c \
FilePath.c \
FileReader.c \
EnumParser.c \
EventsActivation.c \
ProbesActivation.c \
ErrorAgent.c \
EventsDef.c \
EventAgent.c \
ProbesDef.c \
ProbeHolder.c \
ProbeAgent.c \
EnvironmentAgent.c

libplatine_env_plane_la_h = \
unused.h \
ProtoConstants_e.h \
DominoConstants_e.h \
EnumCouple_e.h \
Trace_e.h \
Error_e.h \
Event_e.h \
Time_e.h \
ComParameters_e.h \
IPAddr_e.h \
GenericPacket_e.h \
GenericPort_e.h \
UDPSocket_e.h \
SizedBuffer_e.h \
CircularBuffer_e.h \
LineParser_e.h \
FileInfos_e.h \
FilePath_e.h \
FileReader_e.h \
EnumParser_e.h \
EventsActivation_e.h \
ProbesActivation_e.h \
ErrorAgent_e.h \
EventsDef_e.h \
EventAgent_e.h \
ProbesDef_e.h \
ProbeHolder_e.h \
ProbeAgent_e.h \
EnvironmentAgent_e.h

libplatine_env_plane_la_SOURCES = $(libplatine_env_plane_la_c) $(libplatine_env_plane_la_h)
libplatine_env_plane_la_LIBADD = -lm -lrt

TestEnvironmentAgent_SOURCES =  TestEnvironmentAgent.c
TestEnvironmentAgent_LDADD = libplatine_env_plane.la

ErrorController_SOURCES = \
ErrorController.c \
ErrorControllerInterface.c \
ErrorDef.c \
ErrorOutputFormatter.c

ErrorController_INCLUDES = \
ErrorController_e.h \
ErrorControllerInterface_e.h \
ErrorDef_e.h \
ErrorOutputFormatter_e.h
 
ErrorController_LDADD = libplatine_env_plane.la
# SNMP ErrorController_LDADD = libplatine_env_plane.la -lpthread

EventController_SOURCES = \
EventController.c \
EventControllerInterface.c \
EventOutputFormatter.c

EventController_INCLUDES = \
EventController_e.h \
EventControllerInterface_e.h \
EventOutputFormatter_e.h

EventController_LDADD = libplatine_env_plane.la
# SNMP EventController_LDADD = libplatine_env_plane.la -lpthread


ProbeController_SOURCES = \
ProbeController.c \
ProbeControllerInterface.c

ProbeController_INCLUDES = \
ProbeController_e.h \
ProbeControllerInterface_e.h

ProbeController_LDADD = libplatine_env_plane.la 
# SNMP ProbeController_LDADD = libplatine_env_plane.la -lpthread


AM_CPPFLAGS= -g -Wall -Werror -DUTI_DEBUG_ON -D_ASP_TRACE -D_ASP_PROBE_CHECK  -D_ASP_TRACE_ERROR

libplatine_env_plane_includedir = ${includedir}/platine_env_plane

libplatine_env_plane_include_HEADERS = \
EnvironmentAgent_e.h \
Types_e.h \
ErrorAgent_e.h \
EventAgent_e.h \
ProbeAgent_e.h \
DominoConstants_e.h \
Error_e.h \
IPAddr_e.h \
GenericPort_e.h \
GenericPacket_e.h \
Trace_e.h \
UDPSocket_e.h \
Event_e.h \
EventsActivation_e.h \
Probe_e.h \
ProbesDef_e.h \
ProbeHolder_e.h \
EnumCouple_e.h \
CircularBuffer_e.h \
SizedBuffer_e.h

