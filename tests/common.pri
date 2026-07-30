# Shared configuration for all unit test binaries.
#
# Each test links only the sources it actually exercises, so the suite stays
# fast and does not drag in Backend / drivers / the Python engine.

QT += testlib
QT -= gui

CONFIG += console testcase c++20
CONFIG -= app_bundle

TEMPLATE = app

SRC_DIR = $$PWD/../src
# src/src.pro also puts core/ on the include path; headers there include each
# other by bare name (e.g. CanDbSignal.h -> "BusMessage.h"), so mirror it.
INCLUDEPATH += $$SRC_DIR $$SRC_DIR/core

OBJECTS_DIR = $$PWD/../build/tests/o/$$TARGET
MOC_DIR = $$PWD/../build/tests/moc/$$TARGET
DESTDIR = $$PWD/../build/tests

# qmake only creates these when it generates this Makefile itself; building via
# the top-level subdirs project defers that, so create them explicitly.
!exists($$OBJECTS_DIR): mkpath($$OBJECTS_DIR)
!exists($$MOC_DIR): mkpath($$MOC_DIR)
!exists($$DESTDIR): mkpath($$DESTDIR)
