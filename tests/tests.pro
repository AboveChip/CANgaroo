QT += testlib
QT -= gui

CONFIG += console testcase c++20
CONFIG -= app_bundle

TEMPLATE = app
TARGET = bus_message_signal_test

# Only core/BusMessage is under test; it needs QtCore alone, so the test links a
# couple of sources instead of the whole application.
INCLUDEPATH += $$PWD/../src

SOURCES += \
    $$PWD/BusMessageSignalTest.cpp \
    $$PWD/../src/core/BusMessage.cpp

HEADERS += \
    $$PWD/../src/core/BusMessage.h

OBJECTS_DIR = $$PWD/../build/tests/o
MOC_DIR = $$PWD/../build/tests/moc
DESTDIR = $$PWD/../build/tests

# qmake only creates these when it generates this Makefile itself; building via
# the top-level subdirs project defers that, so create them explicitly.
!exists($$OBJECTS_DIR): mkpath($$OBJECTS_DIR)
!exists($$MOC_DIR): mkpath($$MOC_DIR)
!exists($$DESTDIR): mkpath($$DESTDIR)
