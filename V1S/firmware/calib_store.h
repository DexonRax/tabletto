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

#ifndef CALIB_STORE_H_
#define CALIB_STORE_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t magic;
    uint16_t offset0;
    uint16_t offset1;
} calib_data_t;

#define CALIB_MAGIC 0x54424C54u

bool calib_load(calib_data_t *out);

void calib_save(const calib_data_t *data);

#endif
