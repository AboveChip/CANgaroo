TARGET = ldf_parser_test
include(../common.pri)

# ldf::LdfFile has a member named "signals", which collides with Qt's signals
# macro. no_keywords defines QT_NO_KEYWORDS so the macro is never introduced;
# the test class uses Q_SLOTS instead of "slots" accordingly.
CONFIG += no_keywords

# ldf_parser.h is header-only, so there is nothing else to link.
SOURCES += LdfParserTest.cpp
