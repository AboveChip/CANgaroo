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

// AUTOSAR E2E Profile 2 CRC (CRC-8H2F).
//
// A wrong CRC here is invisible in cangaroo itself -- it only shows up as a real
// ECU silently rejecting every frame the TX generator sends. The anchors are
// therefore external: the check value from the AUTOSAR CRC specification, and a
// bitwise reimplementation that shares no code with the lookup table under test.

#include <QtTest>

#include "core/AutosarE2E.h"
#include "core/BusMessage.h"

namespace
{

// Bitwise CRC-8H2F, deliberately not table driven: poly 0x2F, init 0xFF,
// non-reflected, final xor 0xFF.
[[nodiscard]] uint8_t referenceCrc8H2F(const QByteArray &data, uint8_t init = 0xFF,
                                       bool finalXor = true)
{
    uint8_t crc = init;
    for (const char c : data)
    {
        crc ^= static_cast<uint8_t>(c);
        for (int bit = 0; bit < 8; ++bit)
        {
            crc = (crc & 0x80u) ? static_cast<uint8_t>((crc << 1) ^ 0x2Fu)
                                : static_cast<uint8_t>(crc << 1);
        }
    }
    return finalXor ? static_cast<uint8_t>(crc ^ 0xFFu) : crc;
}

[[nodiscard]] BusMessage messageFromHex(const char *hex)
{
    const QByteArray data = QByteArray::fromHex(QByteArray(hex));
    BusMessage msg;
    msg.setLength(static_cast<uint8_t>(data.size()));
    for (int i = 0; i < data.size(); i++)
    {
        msg.setByte(static_cast<uint8_t>(i), static_cast<uint8_t>(data.at(i)));
    }
    return msg;
}

} // namespace

class AutosarE2ETest : public QObject
{
    Q_OBJECT

private slots:
    void specCheckValue();
    void tableMatchesBitwiseImplementation();

    void profile2Crc_data();
    void profile2Crc();

    void crcIgnoresByteZero();
    void crcDependsOnCounterNibble();
    void crcDependsOnDataId();
    void crcDependsOnLength();
    void crcCoversFullFdPayload();
};

// The AUTOSAR CRC specification gives 0xDF as the CRC-8H2F check value for the
// ASCII string "123456789" (init 0xFF, final xor 0xFF). If the polynomial or the
// table is wrong, this is the assertion that says so.
void AutosarE2ETest::specCheckValue()
{
    const QByteArray input("123456789");

    uint8_t crc = 0xFFu;
    for (const char c : input)
    {
        crc = crc8h2f_byte(crc, static_cast<uint8_t>(c));
    }

    QCOMPARE(static_cast<uint8_t>(crc ^ 0xFFu), uint8_t(0xDF));
    QCOMPARE(referenceCrc8H2F(input), uint8_t(0xDF));
}

// The header builds its table with consteval; recompute it bitwise here so a
// mistake in the table generator cannot hide behind itself.
void AutosarE2ETest::tableMatchesBitwiseImplementation()
{
    for (int i = 0; i < 256; ++i)
    {
        uint8_t expected = static_cast<uint8_t>(i);
        for (int bit = 0; bit < 8; ++bit)
        {
            expected = (expected & 0x80u) ? static_cast<uint8_t>((expected << 1) ^ 0x2Fu)
                                          : static_cast<uint8_t>(expected << 1);
        }

        // crc8h2f_byte(0, i) is a plain table lookup at index i.
        QCOMPARE(crc8h2f_byte(0, static_cast<uint8_t>(i)), expected);
    }
}

void AutosarE2ETest::profile2Crc_data()
{
    QTest::addColumn<QString>("payloadHex");
    QTest::addColumn<quint16>("dataId");
    QTest::addColumn<quint8>("expected");

    // Values from an independent bitwise implementation of the Profile 2 input
    // sequence: DataID low, DataID high, 0x00, then data[1..length-1].
    QTest::newRow("all zero, id 0")   << "0000000000000000" << quint16(0x0000) << quint8(0xE4);
    QTest::newRow("all ff, id 0")     << "FFFFFFFFFFFFFFFF" << quint16(0x0000) << quint8(0xDD);
    QTest::newRow("counter only")     << "0003000000000000" << quint16(0x0123) << quint8(0xAC);
    QTest::newRow("ramp")             << "0001020304050607" << quint16(0x0234) << quint8(0xB8);
    QTest::newRow("high data id")     << "000ADEADBEEF0011" << quint16(0xBEEF) << quint8(0x8A);
    QTest::newRow("minimum dlc 2")    << "0005"             << quint16(0x0042) << quint8(0x33);
}

void AutosarE2ETest::profile2Crc()
{
    QFETCH(QString, payloadHex);
    QFETCH(quint16, dataId);
    QFETCH(quint8, expected);

    const BusMessage msg = messageFromHex(qPrintable(payloadHex));
    QCOMPARE(e2e_p2_compute_crc(msg, dataId), static_cast<uint8_t>(expected));
}

// Byte 0 holds the CRC itself, so it must be fed to the CRC as 0x00 regardless
// of what the frame currently carries there. Otherwise the value would depend on
// whatever was in the buffer beforehand.
void AutosarE2ETest::crcIgnoresByteZero()
{
    BusMessage a = messageFromHex("0001020304050607");
    BusMessage b = messageFromHex("FF01020304050607");
    BusMessage c = messageFromHex("A501020304050607");

    const uint8_t crc = e2e_p2_compute_crc(a, 0x1234);
    QCOMPARE(e2e_p2_compute_crc(b, 0x1234), crc);
    QCOMPARE(e2e_p2_compute_crc(c, 0x1234), crc);
}

void AutosarE2ETest::crcDependsOnCounterNibble()
{
    BusMessage msg = messageFromHex("0000000000000000");
    const uint8_t crc0 = e2e_p2_compute_crc(msg, 0x1234);

    msg.setByte(1, 0x01);
    const uint8_t crc1 = e2e_p2_compute_crc(msg, 0x1234);

    QVERIFY(crc0 != crc1);
}

void AutosarE2ETest::crcDependsOnDataId()
{
    const BusMessage msg = messageFromHex("0001020304050607");

    QVERIFY(e2e_p2_compute_crc(msg, 0x0000) != e2e_p2_compute_crc(msg, 0x0001));
    // Both DataID bytes must be mixed in, in the documented low-then-high order.
    QVERIFY(e2e_p2_compute_crc(msg, 0x0100) != e2e_p2_compute_crc(msg, 0x0001));
}

// The CRC runs to the frame's DLC, so a shorter frame must produce a different
// value even when the underlying buffer bytes are identical.
void AutosarE2ETest::crcDependsOnLength()
{
    BusMessage msg = messageFromHex("0001020304050607");
    const uint8_t crc8 = e2e_p2_compute_crc(msg, 0x1234);

    msg.setLength(4);
    const uint8_t crc4 = e2e_p2_compute_crc(msg, 0x1234);

    QVERIFY(crc8 != crc4);
}

void AutosarE2ETest::crcCoversFullFdPayload()
{
    BusMessage msg;
    msg.setLength(64);
    msg.setByte(0, 0x00);
    for (uint8_t i = 1; i < 64; i++)
    {
        msg.setByte(i, static_cast<uint8_t>((i * 7) & 0xFF));
    }

    QCOMPARE(e2e_p2_compute_crc(msg, 0x1337), uint8_t(0x90));

    // Changing the very last byte must change the CRC: proof the loop reaches it.
    msg.setByte(63, static_cast<uint8_t>(msg.getByte(63) ^ 0xFF));
    QVERIFY(e2e_p2_compute_crc(msg, 0x1337) != uint8_t(0x90));
}

QTEST_APPLESS_MAIN(AutosarE2ETest)

#include "AutosarE2ETest.moc"
