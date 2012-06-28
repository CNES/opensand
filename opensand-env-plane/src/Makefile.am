bin_PROGRAMS = ErrorController EventController ProbeController  
noinst_PROGRAMS = TestEnvironmentAgent
lib_LTLIBRARIES = libopensand_env_plane.la

libopensand_env_plane_la_c = \
Trace.c \
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

libopensand_env_plane_la_h = \
unused.h \
ProtoConstants_e.h \
DominoConstants_e.h \
EnumCouple_e.h \
Trace_e.h \
TracesDef_e.h \
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

libopensand_env_plane_la_SOURCES = $(libopensand_env_plane_la_c) $(libopensand_env_plane_la_h)
libopensand_env_plane_la_LIBADD = -lm -lrt

TestEnvironmentAgent_SOURCES =  TestEnvironmentAgent.c
TestEnvironmentAgent_LDADD = libopensand_env_plane.la

ErrorController_INCLUDES = \
Controller_e.h \
ErrorController_e.h \
ErrorControllerInterface_e.h \
ErrorDef_e.h \
ErrorOutputFormatter_e.h

ErrorController_SOURCES = \
ErrorController.c \
ErrorControllerInterface.c \
ErrorDef.c \
ErrorOutputFormatter.c \
$(ErrorController_INCLUDES)

 
ErrorController_LDADD = libopensand_env_plane.la
# SNMP ErrorController_LDADD = libopensand_env_plane.la -lpthread

EventController_INCLUDES = \
EventController_e.h \
EventControllerInterface_e.h \
EventOutputFormatter_e.h

EventController_SOURCES = \
EventController.c \
EventControllerInterface.c \
EventOutputFormatter.c \
$(EventController_INCLUDES)


EventController_LDADD = libopensand_env_plane.la
# SNMP EventController_LDADD = libopensand_env_plane.la -lpthread


ProbeController_INCLUDES = \
ProbeController_e.h \
ProbeControllerInterface_e.h

ProbeController_SOURCES = \
ProbeController.c \
ProbeControllerInterface.c \
$(ProbeController_INCLUDES)

ProbeController_LDADD = libopensand_env_plane.la 
# SNMP ProbeController_LDADD = libopensand_env_plane.la -lpthread


libopensand_env_plane_includedir = ${includedir}/opensand_env_plane

libopensand_env_plane_include_HEADERS = \
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

