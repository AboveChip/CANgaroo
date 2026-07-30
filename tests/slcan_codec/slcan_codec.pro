TARGET = slcan_codec_test
include(../common.pri)

# SlcanFrameCodec.h is header-only; it needs BusMessage only.
SOURCES += \
    SlcanCodecTest.cpp \
    $$SRC_DIR/core/BusMessage.cpp
