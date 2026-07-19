# Tabletto — dual-licensed under AGPLv3 for noncommercial use, or a separate
# commercial license for manufacturing/sale. See LICENSE and
# LICENSE-COMMERCIAL.md in the repo root, or contact the author.
#
# Copyright (C) 2026 Dexon Rax
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
 
import struct
import threading
import time
import json
import os

import serial
from pynput import keyboard, mouse

# ============ CONFIGURATION - adjust for your hardware ============

SERIAL_PORT = "/dev/ttyACM0"   # check `arduino-cli board list` or dmesg
BAUD_RATE = 115200

FRAME_MARKER = 0xAA
FRAME_SIZE = 5  # marker(1) + angleA(2) + angleB(2), little-endian

L1_MM = 65.0   # length of first arm (base -> elbow), in mm
L2_MM = 65.0   # length of second arm (elbow -> tip), in mm

WORKSPACE_WIDTH_MM = 100.0
WORKSPACE_HEIGHT_MM = 80.0

# Base on the right side, centered vertically, shifted 13.6mm left
# from the right edge of the workspace (inward)
BASE_OFFSET_X_MM = -17.2
BASE_OFFSET_Y_MM = WORKSPACE_HEIGHT_MM / 2

# Active area - subregion of the workspace mapped to the full screen.
# Coordinates are in the "workspace" system (0,0 = top-left corner of the
# workspace, x right, y down) - NOT relative to the arm base.
ACTIVE_AREA_OFFSET_X_MM = 30.0
ACTIVE_AREA_OFFSET_Y_MM = 10.0
ACTIVE_AREA_WIDTH_MM = 64.8
ACTIVE_AREA_HEIGHT_MM = 54.0

MOUSE_HOTKEY = "<ctrl>+1"
CALIBRATE_HOTKEY = "<ctrl>+2"   # replaces the [c] key from GUI version

# Encoder angle increase direction - if rotating right decreases the angle
# instead of increasing, change to -1
DIR_A = 1
DIR_B = 1

CALIBRATION_FILE = "calibration.json"

POLL_INTERVAL_S = 0.001  # 1ms

# How often to print status to console (to avoid flooding stdout)
STATUS_PRINT_EVERY_N = 200  # with 1ms polling this is ~5Hz

# ==================================================================

TICKS_PER_REV = 4096.0
ERR_VALUE = 0xFFFF


class AS5600Reader:
    """Reads binary frames from the serial port in a separate thread, keeps the latest value.
    Frame format (firmware): [0xAA][angleA_lo][angleA_hi][angleB_lo][angleB_hi]"""

    def __init__(self, port, baud):
        self.ser = serial.Serial(port, baud, timeout=1)
        self.angle_a = None
        self.angle_b = None
        self.lock = threading.Lock()
        self.running = True
        self.thread = threading.Thread(target=self._loop, daemon=True)
        self.thread.start()

    def _loop(self):
        while self.running:
            try:
                byte = self.ser.read(1)
            except Exception:
                continue
            if not byte or byte[0] != FRAME_MARKER:
                continue  # look for marker - resync after potential sync loss

            payload = self.ser.read(FRAME_SIZE - 1)
            if len(payload) != FRAME_SIZE - 1:
                continue  # incomplete frame (timeout) - wait for next marker

            a, b = struct.unpack("<HH", payload)
            if a == ERR_VALUE or b == ERR_VALUE:
                continue  # read error on one of the busses in firmware

            with self.lock:
                self.angle_a = a
                self.angle_b = b

    def latest(self):
        with self.lock:
            return self.angle_a, self.angle_b

    def stop(self):
        self.running = False
        self.ser.close()


class Calibration:
    """Offsets in raw_angle ticks corresponding to theta1=0, theta2=0 position
    (arm fully extended to the side)."""

    def __init__(self):
        self.offset_a = 0
        self.offset_b = 0
        self.done = False
        self.load_from_file()

    def calibrate(self, raw_a, raw_b):
        self.offset_a = raw_a
        self.offset_b = raw_b
        self.done = True
        self.save_to_file()

    def save_to_file(self):
        try:
            data = {"offset_a": self.offset_a, "offset_b": self.offset_b}
            with open(CALIBRATION_FILE, "w") as f:
                json.dump(data, f)
            print(f"[tabletto] Calibration saved to file: {CALIBRATION_FILE}")
        except Exception as e:
            print(f"[tabletto] Calibration save error: {e}")

    def load_from_file(self):
        if os.path.exists(CALIBRATION_FILE):
            try:
                with open(CALIBRATION_FILE, "r") as f:
                    data = json.load(f)
                self.offset_a = data["offset_a"]
                self.offset_b = data["offset_b"]
                self.done = True
                print(f"[tabletto] Loaded calibration from file: {data}")
            except Exception as e:
                print(f"[tabletto] Calibration read error: {e}")
        else:
            print(f"[tabletto] No calibration file. Manual zeroing required [{CALIBRATE_HOTKEY}].")

    def to_radians(self, raw_a, raw_b):
        import math
        da = ((raw_a - self.offset_a) % TICKS_PER_REV)
        db = ((raw_b - self.offset_b) % TICKS_PER_REV)
        # normalize to range -180..180 degrees
        if da > TICKS_PER_REV / 2:
            da -= TICKS_PER_REV
        if db > TICKS_PER_REV / 2:
            db -= TICKS_PER_REV
        theta1 = DIR_A * (da / TICKS_PER_REV) * 2 * math.pi
        theta2 = DIR_B * (db / TICKS_PER_REV) * 2 * math.pi
        return theta1, theta2


def forward_kinematics(theta1, theta2):
    """theta1: angle of the first arm from the X axis (calibration position = 0)
    theta2: angle of the second arm RELATIVE to the first (elbow angle)
    Returns (x_mm, y_mm) relative to the ARM BASE (not workspace)."""
    import math
    x = L1_MM * math.cos(theta1) + L2_MM * math.cos(theta1 + theta2)
    y = L1_MM * math.sin(theta1) + L2_MM * math.sin(theta1 + theta2)
    return x, y


def arm_to_workspace(x_mm, y_mm):
    """Converts coordinates relative to the arm base to 'workspace'
    coordinates: (0,0) = top-left corner of the workspace, x right,
    y DOWN (like on screen/canvas). This is the coordinate system
    to think in - not relative to the base, which sits at mid-height."""
    x_ws = x_mm + BASE_OFFSET_X_MM
    y_ws = y_mm + BASE_OFFSET_Y_MM
    return x_ws, y_ws


def workspace_to_screen(x_ws, y_ws, screen_w, screen_h):
    """Maps workspace coordinates to screen pixels, constrained
    to the ACTIVE_AREA_* rectangle. Returns (screen_x, screen_y, in_bounds)."""
    nx = (x_ws - ACTIVE_AREA_OFFSET_X_MM) / ACTIVE_AREA_WIDTH_MM
    ny = (y_ws - ACTIVE_AREA_OFFSET_Y_MM) / ACTIVE_AREA_HEIGHT_MM
    in_bounds = 0.0 <= nx <= 1.0 and 0.0 <= ny <= 1.0
    nx_clamped = max(0.0, min(1.0, nx))
    ny_clamped = max(0.0, min(1.0, ny))
    return nx_clamped * screen_w, ny_clamped * screen_h, in_bounds


class MouseController:
    """Toggles system cursor control with a global hotkey.
    The movement (move_to) is called from the main loop, toggle()
    is called from a separate keyboard listener thread (pynput)."""

    def __init__(self):
        self._mouse = mouse.Controller()
        self.active = False
        self.lock = threading.Lock()

    def toggle(self):
        with self.lock:
            self.active = not self.active
            state = self.active
        print(f"[tabletto] Mouse control: {'ENABLED' if state else 'disabled'}")

    def is_active(self):
        with self.lock:
            return self.active

    def move_to(self, screen_x, screen_y):
        if self.is_active():
            self._mouse.position = (int(screen_x), int(screen_y))


def get_screen_size():
    """Screen size without tkinter dependency (Xlib on X11)."""
    try:
        from Xlib import display
        d = display.Display()
        screen = d.screen()
        return screen.width_in_pixels, screen.height_in_pixels
    except Exception:
        print("[tabletto] Could not read screen size via Xlib, "
              "using default 1920x1080. Install python-xlib "
              "or set SCREEN_W/SCREEN_H manually in code.")
        return 1920, 1080


def calibration_requested_flag():
    """Simple signaling mechanism between the hotkey thread and the main loop."""
    return threading.Event()


def main():
    reader = AS5600Reader(SERIAL_PORT, BAUD_RATE)
    mouse_ctl = MouseController()
    cal = Calibration()

    screen_w, screen_h = get_screen_size()

    calibrate_flag = threading.Event()

    def request_calibration():
        calibrate_flag.set()

    hotkey_listener = keyboard.GlobalHotKeys({
        MOUSE_HOTKEY: mouse_ctl.toggle,
        CALIBRATE_HOTKEY: request_calibration,
    })
    hotkey_listener.start()

    print("[tabletto] Driver started (headless, polling 1ms).")
    print(f"[tabletto] {MOUSE_HOTKEY} = toggle mouse, {CALIBRATE_HOTKEY} = calibrate "
          f"(extend arm sideways before pressing)")

    last_in_bounds = None
    loop_count = 0

    try:
        while True:
            loop_start = time.perf_counter()

            if calibrate_flag.is_set():
                calibrate_flag.clear()
                raw_a, raw_b = reader.latest()
                if raw_a is None or raw_b is None:
                    print("[tabletto] NO DATA FROM PORT - check SERIAL_PORT / firmware")
                else:
                    cal.calibrate(raw_a, raw_b)

            raw_a, raw_b = reader.latest()

            if raw_a is not None and cal.done:
                theta1, theta2 = cal.to_radians(raw_a, raw_b)
                x_mm, y_mm = forward_kinematics(theta1, theta2)
                x_ws, y_ws = arm_to_workspace(x_mm, y_mm)

                screen_x, screen_y, in_bounds = workspace_to_screen(
                    x_ws, y_ws, screen_w, screen_h
                )
                mouse_ctl.move_to(screen_x, screen_y)

                if in_bounds != last_in_bounds:
                    state = "" if in_bounds else " (OUTSIDE ACTIVE AREA)"
                    #print(f"[tabletto] x={x_ws:6.1f}mm y={y_ws:6.1f}mm{state}")
                    last_in_bounds = in_bounds

                if loop_count % STATUS_PRINT_EVERY_N == 0:
                    mouse_state = "ACTIVE" if mouse_ctl.is_active() else "disabled"
                    #print(f"[tabletto] x={x_ws:6.1f}mm y={y_ws:6.1f}mm  raw=({raw_a},{raw_b})  "
                          #f"mysz: {mouse_state}")
            elif raw_a is not None and loop_count % STATUS_PRINT_EVERY_N == 0:
                print(f"[tabletto] raw=({raw_a},{raw_b})  NO CALIBRATION - "
                      f"{CALIBRATE_HOTKEY} with arm extended")

            loop_count += 1

            elapsed = time.perf_counter() - loop_start
            sleep_time = POLL_INTERVAL_S - elapsed
            if sleep_time > 0:
                time.sleep(sleep_time)

    except KeyboardInterrupt:
        print("[tabletto] Shutting down...")
    finally:
        reader.stop()
        hotkey_listener.stop()


if __name__ == "__main__":
    main()
