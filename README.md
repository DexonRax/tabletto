# Tabletto V1S

> A ridiculously fast DIY osu! "tablet" built around a **SCARA arm** mechanism, powered by an **RP2040** (RP2040-Zero) and **dual AS5600 magnetic rotary encoders**.

Tabletto V1S was designed with one goal in mind: **the lowest possible input latency** using inexpensive, easy-to-source components, fully 3D printed mechanics, and custom open-source PCBs.

---

# Gallery

![Tabletto image](https://dexonrax.s-ul.eu/iLvSJvkM)

---

# Features

* ⚡ **~1000 Hz report rate** (1 ms USB HID polling)
* 🎯 Minimal cursor jitter
* 🚀 Faster than most commercial graphics tablets
* 🖨️ Fully 3D printable mechanical design
* 🔩 Custom open-source **KiCad PCBs** (base + arm sensor board)
* 🧠 On-device forward kinematics — no Python needed, works with **OpenTabletDriver**
* 🔘 Physical button: **short press** toggles the tablet, **hold 3 s** to calibrate
* 💾 Calibration stored in the RP2040's flash, survives power cycles

---

# How it works

Tabletto uses a **SCARA (Selective Compliance Assembly Robot Arm)** mechanism.

Two **AS5600 magnetic rotary encoders** measure the angle of each arm joint — one lives on the base PCB, the other on a small board at the elbow. The RP2040 firmware reads both encoders over two separate I2C buses and computes the pen position with forward kinematics, then sends it to the host as a vendor-defined HID report.

Because the sensors provide high-resolution angle measurements and the mechanism has very little moving mass, the result is an extremely responsive pointing device well suited for rhythm games like osu!.

---

# Hardware

Required components:

| Quantity | Part                            |
| -------- | ------------------------------- |
| 1        | RP2040-Zero (or similar module) |
| 2        | AS5600 Magnetic Rotary Encoder  |
| 2        | F693ZZ 3x8x4mm Bearings         |
| 4        | 4.7kΩ Resistor                  |
| 1        | 100nF Capacitor                 |
| 1        | JST XH 2.54mm straight connector|
| 1        | JST XH 2.54mm male cable        |
| 1        | Keyboard Switch                 |
| 4        | 10mm M3 Screws                  |
| —        | Magnets - come with AS5600      |
| —        | 3D Printed Parts                |
| —        | PCBs - from `V1S/pcb`           |

Both boards have **3.2 mm M3 mounting holes**.

---

# PCBs

The electronic design is open source and lives in [V1S/pcb](V1S/pcb):

* **`pcb/base`** — base board: RP2040-Zero module, one AS5600, 4.7kΩ I2C pull-ups, calibration button, and a 4-pin connector to the arm board.
* **`pcb/arm`** — elbow board: the second AS5600 with a 100 nF decoupling cap, connected over a short 4-pin cable.

Open the `.kicad_sch` / `.kicad_pcb` files in KiCad 8+ and order them from any PCB fab (both boards are two-layer).

## Firmware pin mapping

| Function        | Pin  |
| --------------- | ---- |
| I2C0 SDA (base) | GP0  |
| I2C0 SCL (base) | GP1  |
| I2C1 SDA (arm)  | GP2  |
| I2C1 SCL (arm)  | GP3  |
| Calibration btn | GP4  |

---

# Firmware

Located in [V1S/firmware](V1S/firmware). Built with the **Raspberry Pi Pico SDK** and **TinyUSB**.

## Flashing (easiest)

A precompiled build is included at `V1S/firmware/precompiled/tabletto.uf2`:

1. Plug the RP2040-Zero into USB while holding **BOOT**
2. Drag and drop `tabletto.uf2` onto the USB drive that appears
3. Done — the device enumerates as a HID tablet (VID `0x1209`, PID `0x0001`)

## Building from source

Requires the Pico SDK and CMake:

```
cd V1S/firmware
cmake -B build -DPICO_SDK_PATH=/path/to/pico-sdk
make -C build
```

The resulting `build/tabletto.uf2` can be flashed as above.

---

# Driver

Tabletto exposes a standard vendor-defined HID report, so **no custom driver is needed** — just use [OpenTabletDriver](https://github.com/OpenTabletDriver/OpenTabletDriver).

Import the configuration from `V1S/driver/OpenTabletDriver Configuration/TablettoV1S.json` 
into OpenTabletDriver (restart needed): 
- Linux: ~/.local/share/OpenTabletDriver/Configurations/ (create it if missing: mkdir -p ~/.local/share/OpenTabletDriver/Configurations)
- Windows: %LOCALAPPDATA%\OpenTabletDriver\Configurations

It defines:
* Work area: **180 × 112.5 mm** (X 0–18000, Y 0–11250, 0.01 mm units)
* Input report length: 9 bytes
* `libinputoverride: 1` for reliable Linux behavior

---

# Calibration

1. With the tablet connected, **pull the arm straight down** (toward you), so both sensors read their reference position.
2. **Hold the button for 3 seconds.**
3. Short-press the button at any time to toggle the tablet on/off.

Direction tuning lives in the firmware (`X_DIR` / `Y_DIR` in `main.c`) if the axes ever need flipping.

---

# Why SCARA?

Unlike traditional graphics tablets, Tabletto directly measures arm angles with magnetic encoders.

Advantages include:

* extremely low latency
* high update rate
* no pressure to use specialized tablet hardware
* inexpensive components
* entirely repairable and printable

---

# 3D Printing

All `.stl` files can be found in [V1S/models](V1S/models):

| File                  | Purpose                           |
| --------------------- | --------------------------------- |
| `arm1_up.stl`         | Arm segment 1 (magnet-up variant) |
| `arm1_down.stl`       | Arm segment 1 (magnet-down variant) |
| `arm2.stl`            | Arm segment 2 (elbow to pen)      |
| `base_down_workarea.stl` | Bottom side of the base |
| `base_up_debug.stl`   | Upper side of the base (debug has a cutout for RP2040 buttons) |
| `workarea.stl`        | Flat work area plate              |
| `offset_nib_3.6mm.stl` | Nut-like offset for spacing (6 needed)    |

Every structural component is designed to be **3D printed** — no CNC machining or laser cutting required. Place the magnets that came with the AS5600s on top of the bearings.

---

# Repository layout

```
V1S/
├── driver/    OpenTabletDriver configuration (JSON)
├── firmware/  RP2040 firmware (Pico SDK + TinyUSB) + precompiled UF2
├── models/    3D printable STL files
└── pcb/       KiCad sources: base board + arm sensor board
```

---

# License

Copyright (C) 2026 Dexon Rax

Tabletto is free and open source (AGPLv3) for personal and noncommercial
use — this covers the RP2040 firmware, the OpenTabletDriver configuration,
the 3D printable models, and the KiCad PCB sources.

💡 Want to manufacture or sell Tabletto commercially, or use it in a
closed product? Contact me for licensing — see LICENSE-COMMERCIAL.md.
