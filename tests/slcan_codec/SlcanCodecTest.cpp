/*

  Copyright (c) 2026 Schildkroet

  This file is part of cangaroo.

  cangaroo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  cangaroo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with cangaroo.  If not, see <http://www.gnu.org/licenses/>.

*/

// SLCAN (Lawicel) ASCII frame decoding.
//
// This is attacker-adjacent code in the sense that matters for a bus tool: the
// bytes come from a device over a serial link and may be truncated, mistimed or
// garbage. The malformed-input cases below are the point of the file -- a short
// line must be rejected, never read past its end.

#include <QtTest>

#include "core/BusMessage.h"
#include "driver/SLCANDriver/SlcanFrameCodec.h"

class SlcanCodecTest : public QObject
{
    Q_OBJECT

private slots:
    void decodesStandardDataFrame();
    void decodesExtendedDataFrame();
    void decodesRemoteFrames();
    void remoteFrameWithNonZeroDlcIsAccepted();
    void remoteFrameDataAreaIsZeroed();
    void remoteFrameIgnoresTrailingPayload();
    void decodesZeroLengthFrame();
    void decodesLowercaseHex();
    void standardIdAboveElevenBitsIsMasked();

    void decodesFdFrame();
    void decodesFdWithBitrateSwitch();
    void fdDlcNibbleMapping_data();
    void fdDlcNibbleMapping();

    void rejectsEmptyLine();
    void rejectsUnknownTypeCharacter();
    void rejectsShortLine();
    void rejectsTruncatedPayload();
    void rejectsNonHexIdentifier();
    void rejectsNonHexPayload();
    void rejectsClassicDlcAboveEight();
    void rejectsFdDlcAboveFifteen();

    void dlcNibbleRoundTrip_data();
    void dlcNibbleRoundTrip();
    void hexNibbleEncoding();

    void encodesTypeCharacter_data();
    void encodesTypeCharacter();
    void encodesStandardDataFrame();
    void encodesExtendedFdFrame();
    void encodeRejectsFdRemoteFrame();
    void encodeDecodeRoundTrip_data();
    void encodeDecodeRoundTrip();
};

void SlcanCodecTest::decodesStandardDataFrame()
{
    BusMessage msg;
    QVERIFY(slcan::parseFrameLine("t1234DEADBEEF", msg));

    QCOMPARE(msg.getId(), 0x123u);
    QVERIFY(!msg.isExtended());
    QVERIFY(!msg.isRTR());
    QVERIFY(!msg.isFD());
    QVERIFY(!msg.isBRS());
    QCOMPARE(msg.getLength(), uint8_t(4));
    QCOMPARE(msg.getByte(0), uint8_t(0xDE));
    QCOMPARE(msg.getByte(1), uint8_t(0xAD));
    QCOMPARE(msg.getByte(2), uint8_t(0xBE));
    QCOMPARE(msg.getByte(3), uint8_t(0xEF));
}

void SlcanCodecTest::decodesExtendedDataFrame()
{
    BusMessage msg;
    QVERIFY(slcan::parseFrameLine("T1FFFFFFF80011223344556677", msg));

    QCOMPARE(msg.getId(), 0x1FFFFFFFu);
    QVERIFY(msg.isExtended());
    QVERIFY(!msg.isRTR());
    QCOMPARE(msg.getLength(), uint8_t(8));
    QCOMPARE(msg.getByte(0), uint8_t(0x00));
    QCOMPARE(msg.getByte(7), uint8_t(0x77));
}

void SlcanCodecTest::decodesRemoteFrames()
{
    // A remote frame with DLC 0 works.
    BusMessage zero;
    QVERIFY(slcan::parseFrameLine("r1230", zero));
    QCOMPARE(zero.getId(), 0x123u);
    QVERIFY(zero.isRTR());
    QVERIFY(!zero.isExtended());
    QCOMPARE(zero.getLength(), uint8_t(0));

    BusMessage ext;
    QVERIFY(slcan::parseFrameLine("R000004000", ext));
    QCOMPARE(ext.getId(), 0x400u);
    QVERIFY(ext.isRTR());
    QVERIFY(ext.isExtended());
    QCOMPARE(ext.getLength(), uint8_t(0));
}

// A remote frame requests data rather than carrying it, so "r1238" is a complete
// SLCAN line: "remote request for 8 bytes from id 0x123". The DLC must be kept as
// the requested length while no payload digits are expected. Earlier revisions
// demanded 16 payload hex digits here and dropped every such frame as an RX error.
void SlcanCodecTest::remoteFrameWithNonZeroDlcIsAccepted()
{
    BusMessage msg;
    QVERIFY(slcan::parseFrameLine("r1238", msg));
    QCOMPARE(msg.getId(), 0x123u);
    QVERIFY(msg.isRTR());
    QVERIFY(!msg.isExtended());
    QCOMPARE(msg.getLength(), uint8_t(8));

    BusMessage ext;
    QVERIFY(slcan::parseFrameLine("R1FFFFFFF4", ext));
    QCOMPARE(ext.getId(), 0x1FFFFFFFu);
    QVERIFY(ext.isRTR());
    QVERIFY(ext.isExtended());
    QCOMPARE(ext.getLength(), uint8_t(4));
}

// The requested length is reported, but the data area must not carry anything --
// including leftovers from a previous frame decoded into the same BusMessage.
void SlcanCodecTest::remoteFrameDataAreaIsZeroed()
{
    BusMessage msg;
    QVERIFY(slcan::parseFrameLine("t1238DEADBEEFCAFEF00D", msg));
    QCOMPARE(msg.getByte(0), uint8_t(0xDE));

    QVERIFY(slcan::parseFrameLine("r1238", msg));
    QCOMPARE(msg.getLength(), uint8_t(8));
    for (uint8_t i = 0; i < 8; i++)
    {
        QCOMPARE(msg.getByte(i), uint8_t(0));
    }
}

// Trailing payload on a remote frame is surplus, not a reason to reject the line.
void SlcanCodecTest::remoteFrameIgnoresTrailingPayload()
{
    BusMessage msg;
    QVERIFY(slcan::parseFrameLine("r1232AABB", msg));
    QVERIFY(msg.isRTR());
    QCOMPARE(msg.getLength(), uint8_t(2));
    QCOMPARE(msg.getByte(0), uint8_t(0));
    QCOMPARE(msg.getByte(1), uint8_t(0));
}

void SlcanCodecTest::decodesZeroLengthFrame()
{
    BusMessage msg;
    QVERIFY(slcan::parseFrameLine("t1230", msg));
    QCOMPARE(msg.getId(), 0x123u);
    QCOMPARE(msg.getLength(), uint8_t(0));
}

void SlcanCodecTest::decodesLowercaseHex()
{
    BusMessage msg;
    QVERIFY(slcan::parseFrameLine("t7ab2dead", msg));
    QCOMPARE(msg.getId(), 0x7ABu);
    QCOMPARE(msg.getLength(), uint8_t(2));
    QCOMPARE(msg.getByte(0), uint8_t(0xDE));
    QCOMPARE(msg.getByte(1), uint8_t(0xAD));
}

// A standard-format line can spell an id above the 11-bit range. The frame is
// accepted and the id is masked to 11 bits, because BusMessage::setId() first
// infers "extended" from the magnitude and parseFrameLine then clears that flag.
// Documented rather than endorsed: such a line is malformed SLCAN to begin with.
void SlcanCodecTest::standardIdAboveElevenBitsIsMasked()
{
    BusMessage msg;
    QVERIFY(slcan::parseFrameLine("tabc2dead", msg));
    QVERIFY(!msg.isExtended());
    QCOMPARE(msg.getId(), 0xABCu & 0x7FFu);
}

void SlcanCodecTest::decodesFdFrame()
{
    // DLC nibble 9 means 12 bytes on an FD frame.
    BusMessage msg;
    QVERIFY(slcan::parseFrameLine("d1009000102030405060708090A0B", msg));

    QCOMPARE(msg.getId(), 0x100u);
    QVERIFY(msg.isFD());
    QVERIFY(!msg.isBRS());
    QCOMPARE(msg.getLength(), uint8_t(12));
    QCOMPARE(msg.getByte(0), uint8_t(0x00));
    QCOMPARE(msg.getByte(11), uint8_t(0x0B));
}

void SlcanCodecTest::decodesFdWithBitrateSwitch()
{
    BusMessage msg;
    QVERIFY(slcan::parseFrameLine("b2001AA", msg));

    QCOMPARE(msg.getId(), 0x200u);
    QVERIFY(msg.isFD());
    QVERIFY(msg.isBRS());
    QCOMPARE(msg.getLength(), uint8_t(1));
    QCOMPARE(msg.getByte(0), uint8_t(0xAA));
}

void SlcanCodecTest::fdDlcNibbleMapping_data()
{
    QTest::addColumn<char>("nibble");
    QTest::addColumn<int>("byteCount");

    QTest::newRow("0")  << '0' << 0;
    QTest::newRow("8")  << '8' << 8;
    QTest::newRow("9")  << '9' << 12;
    QTest::newRow("A") << 'A' << 16;
    QTest::newRow("B") << 'B' << 20;
    QTest::newRow("C") << 'C' << 24;
    QTest::newRow("D") << 'D' << 32;
    QTest::newRow("E") << 'E' << 48;
    QTest::newRow("F") << 'F' << 64;
}

// The CAN FD DLC nibble is a code, not a byte count: 9..F mean 12..64 bytes.
void SlcanCodecTest::fdDlcNibbleMapping()
{
    QFETCH(char, nibble);
    QFETCH(int, byteCount);

    QByteArray line = QByteArray("d100") + nibble;
    line.append(QByteArray(byteCount * 2, '5'));   // 0x55 payload bytes

    BusMessage msg;
    QVERIFY(slcan::parseFrameLine(line, msg));
    QCOMPARE(msg.getLength(), static_cast<uint8_t>(byteCount));
    if (byteCount > 0)
    {
        QCOMPARE(msg.getByte(static_cast<uint8_t>(byteCount - 1)), uint8_t(0x55));
    }
}

void SlcanCodecTest::rejectsEmptyLine()
{
    BusMessage msg;
    QVERIFY(!slcan::parseFrameLine(QByteArray(), msg));
}

void SlcanCodecTest::rejectsUnknownTypeCharacter()
{
    BusMessage msg;
    for (const char *line : { "x1238", "z1238", "S6", "F", "1238" })
    {
        QVERIFY2(!slcan::parseFrameLine(line, msg), line);
    }
}

void SlcanCodecTest::rejectsShortLine()
{
    BusMessage msg;
    // Missing DLC, missing id digits, id shorter than an extended id needs.
    for (const char *line : { "t", "t1", "t12", "t123", "T1FFFFFF" })
    {
        QVERIFY2(!slcan::parseFrameLine(line, msg), line);
    }
}

// The important one: a data frame's DLC promises more payload than the line
// carries. (Remote frames are exempt -- they legitimately carry none.)
void SlcanCodecTest::rejectsTruncatedPayload()
{
    BusMessage msg;
    QVERIFY(!slcan::parseFrameLine("t1238DEADBEEF", msg));        // claims 8, has 4
    QVERIFY(!slcan::parseFrameLine("t1234DEADBE", msg));          // claims 4, has 3.5
    QVERIFY(!slcan::parseFrameLine("d100F0011", msg));            // claims 64, has 2
    QVERIFY(!slcan::parseFrameLine("T1FFFFFFF8AABB", msg));       // extended, claims 8
}

void SlcanCodecTest::rejectsNonHexIdentifier()
{
    BusMessage msg;
    QVERIFY(!slcan::parseFrameLine("tG238DEADBEEF", msg));
    QVERIFY(!slcan::parseFrameLine("t1-38DEADBEEF", msg));
    QVERIFY(!slcan::parseFrameLine("t12 4DEADBEEF", msg));
}

void SlcanCodecTest::rejectsNonHexPayload()
{
    BusMessage msg;
    QVERIFY(!slcan::parseFrameLine("t1232DEZZ", msg));
    QVERIFY(!slcan::parseFrameLine("t1231G0", msg));
}

// A classic CAN frame cannot carry more than 8 bytes, so DLC 9..F is invalid
// there even though it is meaningful for FD.
void SlcanCodecTest::rejectsClassicDlcAboveEight()
{
    BusMessage msg;
    for (const char *line : { "t1239", "t123A", "t123F" })
    {
        QVERIFY2(!slcan::parseFrameLine(line, msg), line);
    }
}

void SlcanCodecTest::rejectsFdDlcAboveFifteen()
{
    // 'G' is not a hex digit at all, so it cannot encode a DLC.
    BusMessage msg;
    QVERIFY(!slcan::parseFrameLine("d100G", msg));
}

void SlcanCodecTest::dlcNibbleRoundTrip_data()
{
    QTest::addColumn<int>("byteCount");

    for (const int len : { 0, 1, 8, 12, 16, 20, 24, 32, 48, 64 })
    {
        QTest::newRow(qPrintable(QString::number(len))) << len;
    }
}

// bytesToDlcNibble and k_dlcToBytes must be inverses for the valid FD lengths;
// otherwise a frame sent out comes back with a different length.
void SlcanCodecTest::dlcNibbleRoundTrip()
{
    QFETCH(int, byteCount);

    const uint8_t nibble = slcan::bytesToDlcNibble(byteCount);
    QVERIFY(nibble < 16);
    QCOMPARE(int(slcan::k_dlcToBytes[nibble]), byteCount);
}

void SlcanCodecTest::hexNibbleEncoding()
{
    QCOMPARE(slcan::hexNibble(0), '0');
    QCOMPARE(slcan::hexNibble(9), '9');
    QCOMPARE(slcan::hexNibble(10), 'A');
    QCOMPARE(slcan::hexNibble(15), 'F');

    // hexNibble and fromHexNibble must round trip.
    for (uint8_t v = 0; v < 16; v++)
    {
        QCOMPARE(slcan::fromHexNibble(slcan::hexNibble(v)), int(v));
    }

    QCOMPARE(slcan::fromHexNibble('g'), -1);
    QCOMPARE(slcan::fromHexNibble(' '), -1);
    QCOMPARE(slcan::fromHexNibble('\0'), -1);
}

// The type character carries the ID length, so an extended frame that goes out
// with the lowercase form is parsed by the device as an 11 bit one: the ID is
// read three nibbles short and DLC and payload shift with it.
void SlcanCodecTest::encodesTypeCharacter_data()
{
    QTest::addColumn<bool>("extended");
    QTest::addColumn<bool>("fd");
    QTest::addColumn<bool>("brs");
    QTest::addColumn<bool>("rtr");
    QTest::addColumn<char>("expected");

    QTest::newRow("data std")     << false << false << false << false << 't';
    QTest::newRow("data ext")     << true  << false << false << false << 'T';
    QTest::newRow("remote std")   << false << false << false << true  << 'r';
    QTest::newRow("remote ext")   << true  << false << false << true  << 'R';
    QTest::newRow("fd std")       << false << true  << false << false << 'd';
    QTest::newRow("fd ext")       << true  << true  << false << false << 'D';
    QTest::newRow("fd brs std")   << false << true  << true  << false << 'b';
    QTest::newRow("fd brs ext")   << true  << true  << true  << false << 'B';
}

void SlcanCodecTest::encodesTypeCharacter()
{
    QFETCH(bool, extended);
    QFETCH(bool, fd);
    QFETCH(bool, brs);
    QFETCH(bool, rtr);
    QFETCH(char, expected);

    BusMessage msg;
    msg.setId(0x123);
    msg.setExtended(extended);
    msg.setFD(fd);
    msg.setBRS(brs);
    msg.setRTR(rtr);
    msg.setLength(1);
    msg.setByte(0, 0xAB);

    const QByteArray line = slcan::buildFrameLine(msg);
    QVERIFY(!line.isEmpty());
    QCOMPARE(line.at(0), expected);

    // An extended frame must carry 8 ID nibbles, a standard one 3.
    QCOMPARE(line.size(), 1 + (extended ? 8 : 3) + 1 + 2 + 1);
}

void SlcanCodecTest::encodesStandardDataFrame()
{
    BusMessage msg;
    msg.setId(0x123);
    msg.setLength(2);
    msg.setByte(0, 0xDE);
    msg.setByte(1, 0xAD);

    QCOMPARE(slcan::buildFrameLine(msg), QByteArray("t1232DEAD\r"));
}

void SlcanCodecTest::encodesExtendedFdFrame()
{
    BusMessage msg;
    msg.setId(0x1FFFFFFF);
    msg.setExtended(true);
    msg.setFD(true);
    msg.setBRS(true);
    msg.setLength(12); // DLC nibble 9

    for (uint8_t i = 0; i < 12; ++i)
        msg.setByte(i, i);

    QCOMPARE(slcan::buildFrameLine(msg),
             QByteArray("B1FFFFFFF9000102030405060708090A0B\r"));
}

void SlcanCodecTest::encodeRejectsFdRemoteFrame()
{
    BusMessage msg;
    msg.setId(0x123);
    msg.setFD(true);
    msg.setRTR(true);

    QVERIFY(slcan::buildFrameLine(msg).isEmpty());
}

void SlcanCodecTest::encodeDecodeRoundTrip_data()
{
    QTest::addColumn<uint32_t>("id");
    QTest::addColumn<bool>("extended");
    QTest::addColumn<bool>("fd");
    QTest::addColumn<bool>("brs");
    QTest::addColumn<int>("length");

    QTest::newRow("std classic")  << 0x7FFu       << false << false << false << 8;
    QTest::newRow("ext classic")  << 0x1FFFFFFFu  << true  << false << false << 8;
    QTest::newRow("std fd")       << 0x7FFu       << false << true  << false << 24;
    QTest::newRow("ext fd")       << 0x1FFFFFFFu  << true  << true  << false << 24;
    QTest::newRow("ext fd brs")   << 0x18DAF110u  << true  << true  << true  << 64;
}

void SlcanCodecTest::encodeDecodeRoundTrip()
{
    QFETCH(uint32_t, id);
    QFETCH(bool, extended);
    QFETCH(bool, fd);
    QFETCH(bool, brs);
    QFETCH(int, length);

    BusMessage out;
    out.setId(id);
    out.setExtended(extended);
    out.setFD(fd);
    out.setBRS(brs);
    out.setLength(static_cast<uint8_t>(length));
    for (int i = 0; i < length; ++i)
        out.setByte(static_cast<uint8_t>(i), static_cast<uint8_t>(i * 3));

    const QByteArray line = slcan::buildFrameLine(out);
    QVERIFY(!line.isEmpty());
    QCOMPARE(line.back(), '\r');

    BusMessage back;
    QVERIFY(slcan::parseFrameLine(line.chopped(1), back));

    QCOMPARE(back.getId(), id);
    QCOMPARE(back.isExtended(), extended);
    QCOMPARE(back.isFD(), fd);
    QCOMPARE(back.isBRS(), brs);
    QCOMPARE(back.getLength(), uint8_t(length));
    for (int i = 0; i < length; ++i)
        QCOMPARE(back.getByte(static_cast<uint8_t>(i)), uint8_t(i * 3));
}

QTEST_APPLESS_MAIN(SlcanCodecTest)

#include "SlcanCodecTest.moc"
