# Tabletto V1S firmware - RP2040 SCARA tablet
# Copyright (C) 2026 Dexon Rax <dexonrax@gmail.com>
# SPDX-License-Identifier: AGPL-3.0-or-later
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(tabletto C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(tabletto
    main.c
    usb_descriptors.c
    calib_store.c
)

target_include_directories(tabletto PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(tabletto
    pico_stdlib
    hardware_i2c
    hardware_flash
    hardware_sync
    tinyusb_device
    tinyusb_board
)

pico_enable_stdio_usb(tabletto 0)
pico_enable_stdio_uart(tabletto 1)

pico_add_extra_outputs(tabletto)
