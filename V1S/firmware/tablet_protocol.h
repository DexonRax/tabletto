// Tabletto V1S firmware - RP2040 SCARA tablet
// Copyright (C) 2026 Dexon Rax <dexonrax@gmail.com>
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef TABLET_PROTOCOL_H_
#define TABLET_PROTOCOL_H_

#include <stdint.h>

#define TABLET_REPORT_SIZE 8

typedef struct __attribute__((packed)) {
    uint8_t  buttons;    // bit1 = tip / in-range (OTD TabletReportParser: PenButtons[0] = bit1)
    uint16_t x;
    uint16_t y;
    uint16_t pressure;
    uint8_t  pad;
} tablet_report_t;

_Static_assert(sizeof(tablet_report_t) == TABLET_REPORT_SIZE, "tablet_report_t must be 8 bytes");

#endif
