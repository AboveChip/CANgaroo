TARGET = autosar_e2e_test
include(../common.pri)

SOURCES += \
    AutosarE2ETest.cpp \
    $$SRC_DIR/core/BusMessage.cpp
