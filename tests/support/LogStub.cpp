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

// Test stub for core/Log.
//
// The real implementation (core/Log.cpp) forwards every message to
// Backend::instance(), which would drag the whole application -- drivers,
// windows, the Python engine -- into every test binary that touches code with a
// log call in it. These no-ops satisfy the linker instead.

#include "core/Log.h"

void log_msg(const QDateTime, const log_level_t, const QString) {}
void log_msg(const log_level_t, const QString) {}

void log_debug(const QString) {}
void log_info(const QString) {}
void log_warning(const QString) {}
void log_error(const QString) {}
void log_critical(const QString) {}
void log_fatal(const QString) {}
