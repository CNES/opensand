################################################################################
#   Name       : Makefile
#   Author     : Aurélien DELRIEU <aurelien.delrieu@viveris.fr>
#   Description: create the configuration library for OpenSAND tools
################################################################################

SUBDIRS = \
    . \
    tests \
    pymodule

lib_LTLIBRARIES = libopensand_conf.la

libopensand_conf_la_cpp = \
    BaseElement.cpp \
    BaseEnum.cpp \
    NamedElement.cpp \
    Path.cpp \
    Data.cpp \
    DataValue.cpp \
    DataType.cpp \
    DataEnumType.cpp \
    DataTypesList.cpp \
    DataElement.cpp \
    DataContainer.cpp \
    DataParameter.cpp \
    DataList.cpp \
    DataComponent.cpp \
    DataModel.cpp \
    MetaType.cpp \
    MetaEnumType.cpp \
    MetaTypesList.cpp \
    MetaElement.cpp \
    MetaContainer.cpp \
    MetaParameter.cpp \
    MetaList.cpp \
    MetaComponent.cpp \
    MetaModel.cpp \
    Configuration.cpp

libopensand_conf_la_h = \
    BaseElement.h \
    BaseEnum.h \
    NamedElement.h \
    Path.h \
    Data.h \
    DataValue.h \
    DataType.h \
    DataValueType.h \
    DataEnumType.h \
    DataTypesList.h \
    DataElement.h \
    DataContainer.h \
    DataParameter.h \
    DataList.h \
    DataComponent.h \
    DataModel.h \
    MetaType.h \
    MetaValueType.h \
    MetaEnumType.h \
    MetaTypesList.h \
    MetaElement.h \
    MetaContainer.h \
    MetaParameter.h \
    MetaList.h \
    MetaComponent.h \
    MetaModel.h \
    Configuration.h

libopensand_conf_la_SOURCES = \
    $(libopensand_conf_la_cpp) \
    $(libopensand_conf_la_h)

libopensand_conf_la_LDFLAGS = \
    $(AM_LDFLAGS)

libopensand_conf_includedir = ${includedir}/opensand_conf
#    ${includedir}/opensand_conf/include

libopensand_conf_include_HEADERS = \
    $(libopensand_conf_la_h)
#    Configuration.h
