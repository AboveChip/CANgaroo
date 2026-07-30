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

#pragma once

// Pure SLCAN (Lawicel) ASCII frame encoding/decoding, split out of
// SLCANInterface so it can be exercised without a serial port. Nothing here
// touches hardware, time, or interface state: a line of ASCII goes in, frame
// fields come out.

#include <array>
#include <cstdint>

#include <QByteArray>

#include "core/BusMessage.h"

namespace slcan
{

constexpr int k_stdIdLen = 3;
constexpr int k_extIdLen = 8;

// CAN FD DLC nibble -> byte count (index is DLC nibble 0x0-0xF)
constexpr std::array<uint8_t, 16> k_dlcToBytes = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64
};

[[nodiscard]] constexpr uint8_t bytesToDlcNibble(int len) noexcept
{
    if (len <= 8)  { return static_cast<uint8_t>(len); }
    if (len <= 12) { return 9; }
    if (len <= 16) { return 10; }
    if (len <= 20) { return 11; }
    if (len <= 24) { return 12; }
    if (len <= 32) { return 13; }
    if (len <= 48) { return 14; }
    return 15;
}

[[nodiscard]] constexpr char hexNibble(uint8_t v) noexcept
{
    return v < 10 ? static_cast<char>('0' + v) : static_cast<char>('A' + v - 10);
}

[[nodiscard]] constexpr int fromHexNibble(char c) noexcept
{
    if (c >= '0' && c <= '9') { return c - '0'; }
    if (c >= 'A' && c <= 'F') { return c - 'A' + 10; }
    if (c >= 'a' && c <= 'f') { return c - 'a' + 10; }
    return -1;
}

// Decodes one SLCAN frame line (without the trailing '\r') into msg.
//
// Sets only the fields the line carries: id, extended/RTR/FD/BRS flags, length
// and payload bytes. Timestamp, interface id and direction are the caller's
// business. Returns false and leaves msg unspecified if the line is not a valid
// frame -- including a data line whose payload is shorter than its DLC claims.
//
// Remote frames ('r'/'R') carry a DLC but no payload; their data area is zeroed.
[[nodiscard]] inline bool parseFrameLine(const QByteArray &line, BusMessage &msg) noexcept
{
    if (line.isEmpty()) { return false; }

    bool isExtended = false;
    bool isRtr = false;
    bool isFd = false;
    bool isBrs = false;

    switch (line.at(0))
    {
        case 't':                                                 break;
        case 'T': isExtended = true;                              break;
        case 'r': isRtr = true;                                   break;
        case 'R': isExtended = true; isRtr = true;                break;
        case 'd': isFd = true;                                    break;
        case 'D': isExtended = true; isFd = true;                 break;
        case 'b': isFd = true; isBrs = true;                      break;
        case 'B': isExtended = true; isFd = true; isBrs = true;    break;
        default:
            return false;
    }

    const int idLen = isExtended ? k_extIdLen : k_stdIdLen;
    const int minLen = 1 + idLen + 1; // type + ID + DLC

    if (line.size() < minLen) { return false; }

    uint32_t id = 0;
    for (int i = 1; i <= idLen; ++i)
    {
        const int nibble = fromHexNibble(line.at(i));
        if (nibble < 0) { return false; }
        id = (id << 4) | static_cast<uint32_t>(nibble);
    }

    const int dlcNibble = fromHexNibble(line.at(1 + idLen));
    if (dlcNibble < 0 || (!isFd && dlcNibble > 8) || (isFd && dlcNibble > 15))
    {
        return false;
    }

    const int dataLen = isFd ? int(k_dlcToBytes[dlcNibble]) : dlcNibble;

    // A remote frame requests data instead of carrying it: its DLC states how
    // many bytes are being asked for, and no payload digits follow it. Demanding
    // payload here would reject every legitimate "r123 8" style line.
    const int payloadBytes = isRtr ? 0 : dataLen;
    const int expectedSize = minLen + payloadBytes * 2;

    if (line.size() < expectedSize) { return false; }

    msg.setId(id);
    msg.setExtended(isExtended);
    msg.setRTR(isRtr);
    msg.setFD(isFd);
    msg.setBRS(isBrs);
    msg.setErrorFrame(false);
    msg.setLength(static_cast<uint8_t>(dataLen));

    int pos = minLen;
    for (int i = 0; i < payloadBytes; ++i)
    {
        const int hi = fromHexNibble(line.at(pos));
        const int lo = fromHexNibble(line.at(pos + 1));
        if (hi < 0 || lo < 0) { return false; }
        msg.setByte(static_cast<uint8_t>(i), static_cast<uint8_t>((hi << 4) | lo));
        pos += 2;
    }

    // Keep a remote frame's data area deterministic rather than inheriting
    // whatever the caller's BusMessage happened to hold.
    for (int i = payloadBytes; i < dataLen; ++i)
    {
        msg.setByte(static_cast<uint8_t>(i), 0);
    }

    return true;
}

} // namespace slcan
