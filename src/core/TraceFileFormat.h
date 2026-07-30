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

// Trace export formats and the mapping between them, file extensions and the
// format names used by the Python API. Kept free of BusTrace so the mapping can
// be tested on its own, and shared so the GUI save dialog and the scripting API
// cannot drift apart.

#include <optional>

#include <QString>
#include <QStringList>

enum class TraceFileFormat
{
    CanDump,    // Linux candump text
    VectorAsc,  // Vector ASCII
    VectorMdf,  // Vector MDF4
    Pcap,       // pcap, LINKTYPE_CAN_SOCKETCAN
    PcapNg      // pcapng, LINKTYPE_CAN_SOCKETCAN
};

// File extension for a format, without the leading dot.
[[nodiscard]] inline QString traceFormatExtension(TraceFileFormat format)
{
    switch (format)
    {
        case TraceFileFormat::CanDump:   return QStringLiteral("candump");
        case TraceFileFormat::VectorAsc: return QStringLiteral("asc");
        case TraceFileFormat::VectorMdf: return QStringLiteral("mf4");
        case TraceFileFormat::Pcap:      return QStringLiteral("pcap");
        case TraceFileFormat::PcapNg:    return QStringLiteral("pcapng");
    }
    return {};
}

// Format name accepted by the scripting API.
[[nodiscard]] inline QString traceFormatName(TraceFileFormat format)
{
    switch (format)
    {
        case TraceFileFormat::CanDump:   return QStringLiteral("candump");
        case TraceFileFormat::VectorAsc: return QStringLiteral("asc");
        case TraceFileFormat::VectorMdf: return QStringLiteral("mdf");
        case TraceFileFormat::Pcap:      return QStringLiteral("pcap");
        case TraceFileFormat::PcapNg:    return QStringLiteral("pcapng");
    }
    return {};
}

[[nodiscard]] inline QStringList supportedTraceFormatNames()
{
    return { QStringLiteral("candump"), QStringLiteral("asc"), QStringLiteral("mdf"),
             QStringLiteral("pcap"), QStringLiteral("pcapng") };
}

// Resolves an explicit format name. Accepts the canonical names above plus a few
// obvious aliases. Returns nullopt for anything else -- callers report the error
// rather than guessing.
[[nodiscard]] inline std::optional<TraceFileFormat> traceFormatFromName(const QString &name)
{
    const QString key = name.trimmed().toLower();

    if (key == QStringLiteral("candump") || key == QStringLiteral("log"))
    {
        return TraceFileFormat::CanDump;
    }
    if (key == QStringLiteral("asc") || key == QStringLiteral("vector_asc"))
    {
        return TraceFileFormat::VectorAsc;
    }
    if (key == QStringLiteral("mdf") || key == QStringLiteral("mf4")
        || key == QStringLiteral("mdf4"))
    {
        return TraceFileFormat::VectorMdf;
    }
    if (key == QStringLiteral("pcap"))
    {
        return TraceFileFormat::Pcap;
    }
    if (key == QStringLiteral("pcapng"))
    {
        return TraceFileFormat::PcapNg;
    }

    return std::nullopt;
}

// Infers the format from a file name's extension. Note that "pcapng" must be
// tested before "pcap", since the latter is a suffix of the former.
[[nodiscard]] inline std::optional<TraceFileFormat> traceFormatFromPath(const QString &path)
{
    const int dot = path.lastIndexOf(QLatin1Char('.'));
    if (dot < 0) { return std::nullopt; }

    const QString suffix = path.mid(dot + 1).toLower();
    if (suffix.isEmpty()) { return std::nullopt; }

    if (suffix == QStringLiteral("pcapng")) { return TraceFileFormat::PcapNg; }
    if (suffix == QStringLiteral("pcap"))   { return TraceFileFormat::Pcap; }
    if (suffix == QStringLiteral("candump") || suffix == QStringLiteral("log"))
    {
        return TraceFileFormat::CanDump;
    }
    if (suffix == QStringLiteral("asc")) { return TraceFileFormat::VectorAsc; }
    if (suffix == QStringLiteral("mf4") || suffix == QStringLiteral("mdf")
        || suffix == QStringLiteral("mdf4"))
    {
        return TraceFileFormat::VectorMdf;
    }

    return std::nullopt;
}
