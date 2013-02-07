TEMPLATE = app
TARGET = tst_cmdlineparser
DESTDIR = ../../../bin
INCLUDEPATH += ../../../src/lib/
DEFINES += SRCDIR=\\\"$$PWD/\\\"

QT = core testlib
CONFIG += depend_includepath testcase
CONFIG   += console
CONFIG   -= app_bundle

SOURCES += tst_cmdlineparser.cpp ../../../src/app/qbs/qbstool.cpp

include(../../../src/lib/use.pri)
include(../../../qbs_version.pri)
include(../../../src/app/qbs/parser/parser.pri)
include(../../../src/app/shared/logging/logging.pri)