TARGET = dbc_parser_test
include(../common.pri)

# CanDb::saveXML takes a QDomDocument, hence QtXml. It ignores its Backend
# argument, so no Backend symbols are pulled in.
QT += xml

SOURCES += \
    DbcParserTest.cpp \
    ../support/LogStub.cpp \
    $$SRC_DIR/parser/dbc/DbcParser.cpp \
    $$SRC_DIR/parser/dbc/DbcTokens.cpp \
    $$SRC_DIR/core/BusMessage.cpp \
    $$SRC_DIR/core/DBC/CanDb.cpp \
    $$SRC_DIR/core/DBC/CanDbMessage.cpp \
    $$SRC_DIR/core/DBC/CanDbNode.cpp \
    $$SRC_DIR/core/DBC/CanDbSignal.cpp
