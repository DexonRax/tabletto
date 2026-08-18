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

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "tusb.h"
#include "tablet_protocol.h"
#include "calib_store.h"

#define I2C0_SDA_PIN  0
#define I2C0_SCL_PIN  1
#define I2C1_SDA_PIN  2
#define I2C1_SCL_PIN  3
#define I2C_BAUD      400000

#define CALIB_BUTTON_PIN 4
#define CALIB_HOLD_MS    3000

#define AS5600_ADDR        0x36
#define REG_RAW_ANGLE_H    0x0C
#define I2C_TIMEOUT_US     1000

static void i2c_recover(i2c_inst_t *bus) {
    uint sda_pin = (bus == i2c0) ? I2C0_SDA_PIN : I2C1_SDA_PIN;
    uint scl_pin = (bus == i2c0) ? I2C0_SCL_PIN : I2C1_SCL_PIN;

    i2c_deinit(bus);

    gpio_init(scl_pin);
    gpio_set_dir(scl_pin, GPIO_OUT);
    gpio_put(scl_pin, 1);

    gpio_init(sda_pin);
    gpio_set_dir(sda_pin, GPIO_IN);
    gpio_pull_up(sda_pin);

    for (int i = 0; i < 9; i++) {
        if (gpio_get(sda_pin)) break;
        gpio_put(scl_pin, 0);
        busy_wait_us_32(5);
        gpio_put(scl_pin, 1);
        busy_wait_us_32(5);
    }

    gpio_set_dir(sda_pin, GPIO_OUT);
    gpio_put(sda_pin, 0);
    gpio_put(scl_pin, 1);
    busy_wait_us_32(5);
    gpio_put(sda_pin, 1);

    i2c_init(bus, I2C_BAUD);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
}

static bool as5600_read_raw(i2c_inst_t *bus, uint16_t *out) {
    uint8_t reg = REG_RAW_ANGLE_H;
    uint8_t buf[2];
    if (i2c_write_timeout_us(bus, AS5600_ADDR, &reg, 1, true, I2C_TIMEOUT_US) != 1) {
        i2c_recover(bus);
        return false;
    }
    if (i2c_read_timeout_us(bus, AS5600_ADDR, buf, 2, false, I2C_TIMEOUT_US) != 2) {
        i2c_recover(bus);
        return false;
    }
    *out = ((uint16_t)(buf[0] & 0x0F) << 8) | buf[1];
    return true;
}

#define LINK1_MM 77.0f
#define LINK2_MM 77.0f
#define REACH_MM (LINK1_MM + LINK2_MM)

#define WORKSPACE_WIDTH_MM   180.0f
#define WORKSPACE_HEIGHT_MM  112.5f
#define WORKSPACE_Y_OFFSET_MM 39.0f

#define MAX_X_COORD 18000
#define MAX_Y_COORD 11250

#define X_DIR (-1.0f)
#define Y_DIR ( 1.0f)

static inline uint16_t wrap12(int32_t v) {
    v %= 4096;
    if (v < 0) v += 4096;
    return (uint16_t) v;
}

static calib_data_t calib;
static tablet_report_t report;
static bool tablet_enabled = true;

static void do_calibration(uint16_t raw0, uint16_t raw1) {
    calib.magic = CALIB_MAGIC;
    calib.offset0 = raw0;
    calib.offset1 = raw1;
    calib_save(&calib);

    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    for (int i = 0; i < 6; i++) {
        gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
        sleep_ms(100);
    }
    gpio_put(PICO_DEFAULT_LED_PIN, tablet_enabled);
}

static void handle_calib_button(uint32_t now_ms, bool sensors_ok, uint16_t raw0, uint16_t raw1) {
    static uint32_t press_start_ms = 0;
    static bool triggered = false;

    bool pressed = !gpio_get(CALIB_BUTTON_PIN); // active low

    if (pressed) {
        if (press_start_ms == 0) {
            press_start_ms = now_ms;
        } else if (!triggered && sensors_ok && (now_ms - press_start_ms >= CALIB_HOLD_MS)) {
            triggered = true;
            do_calibration(raw0, raw1);
        }
    } else {
        if (press_start_ms != 0 && !triggered) {
            tablet_enabled = !tablet_enabled;
            gpio_put(PICO_DEFAULT_LED_PIN, tablet_enabled);
        }
        press_start_ms = 0;
        triggered = false;
    }
}

static void forward_kinematics(float theta1, float theta2, float *x_mm, float *y_mm) {
    *x_mm = LINK1_MM * cosf(theta1) + LINK2_MM * cosf(theta1 + theta2);
    *y_mm = LINK1_MM * sinf(theta1) + LINK2_MM * sinf(theta1 + theta2);
}

static uint16_t clamp_coord(float v, uint16_t maxv) {
    if (v < 0) return 0;
    if (v > maxv) return maxv;
    return (uint16_t) v;
}

static void update_and_send_report(void) {
    uint16_t raw0, raw1;
    bool ok0 = as5600_read_raw(i2c0, &raw0);
    bool ok1 = as5600_read_raw(i2c1, &raw1);

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    handle_calib_button(now_ms, ok0 && ok1, raw0, raw1);

    if (!tablet_enabled) return;

    if (!ok0 || !ok1) {
        report.buttons = 0;
        if (tud_hid_ready()) tud_hid_report(2, &report, sizeof(report));
        return;
    }

    uint16_t cal0 = wrap12((int32_t) raw0 - (int32_t) calib.offset0);
    uint16_t cal1 = wrap12((int32_t) raw1 - (int32_t) calib.offset1);

    float theta1 = cal0 * (2.0f * (float) M_PI / 4096.0f);
    float theta2 = -cal1 * (2.0f * (float) M_PI / 4096.0f);

    float x_mm, y_mm;
    forward_kinematics(theta1, theta2, &x_mm, &y_mm);

    float sx = X_DIR * y_mm;
    float sy = Y_DIR * x_mm;

    float nx = (sx + WORKSPACE_WIDTH_MM * 0.5f) / WORKSPACE_WIDTH_MM * MAX_X_COORD;
    float ny = (sy - WORKSPACE_Y_OFFSET_MM) / WORKSPACE_HEIGHT_MM * MAX_Y_COORD;

    report.buttons   = 0x00;
    report.x         = clamp_coord(nx, MAX_X_COORD);
    report.y         = clamp_coord(ny, MAX_Y_COORD);
    report.pressure  = 0x0000;
    report.pad       = 0;

    if (tud_hid_ready()) {
        tud_hid_report(2, &report, sizeof(report));
    }
}

int main(void) {
    stdio_init_all();
    tusb_init();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, tablet_enabled);

    gpio_init(CALIB_BUTTON_PIN);
    gpio_set_dir(CALIB_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(CALIB_BUTTON_PIN);

    i2c_init(i2c0, I2C_BAUD);
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);

    i2c_init(i2c1, I2C_BAUD);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);

    if (!calib_load(&calib)) {
        calib.magic = CALIB_MAGIC;
        calib.offset0 = 0;
        calib.offset1 = 0;
    }

    memset(&report, 0, sizeof(report));

    const uint32_t REPORT_INTERVAL_MS = 1;
    uint32_t last_report_ms = 0;

    while (true) {
        tud_task();

        uint32_t now_ms = to_ms_since_boot(get_absolute_time());
        if (now_ms - last_report_ms >= REPORT_INTERVAL_MS) {
            last_report_ms = now_ms;
            update_and_send_report();
        }
    }
}
