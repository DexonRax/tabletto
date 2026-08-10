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

#include <string.h>
#include "calib_store.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

bool calib_load(calib_data_t *out) {
    const uint8_t *flash_ptr = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
    calib_data_t tmp;
    memcpy(&tmp, flash_ptr, sizeof(tmp));
    if (tmp.magic != CALIB_MAGIC) {
        return false;
    }
    *out = tmp;
    return true;
}

void calib_save(const calib_data_t *data) {
    uint8_t page_buf[FLASH_PAGE_SIZE];
    memset(page_buf, 0, sizeof(page_buf));
    memcpy(page_buf, data, sizeof(*data));

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, page_buf, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}
