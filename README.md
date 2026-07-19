# Tabletto

> A ridiculously fast DIY osu! "tablet" built around a **SCARA arm** mechanism, powered by an **Arduino Pro Micro** and **dual AS5600 magnetic rotary encoders**.

Tabletto was designed with one goal in mind: **the lowest possible input latency** using inexpensive, easy-to-source components and fully 3D printed mechanics.

---

# Gallery

![Tabletto image](https://dexonrax.s-ul.eu/F8z8WAlE)

---

# Features

* ⚡ **600–700 Hz polling rate**
* 🎯 Minimal cursor jitter
* 🚀 Faster than most commercial graphics tablets
* 🖨️ Fully 3D printable mechanical design
* 🔩 Only **2 M3 screws** hold the entire assembly together
* 🪶 Very low pen friction using a single mouse skate

---

# How it works

Tabletto uses a **SCARA (Selective Compliance Assembly Robot Arm)** mechanism.

Two **AS5600 magnetic rotary encoders** measure the angle of each arm joint. The Arduino Pro Micro continuously reads both encoders, then python driver calculates the pen position using forward kinematics, and sends the maps the cursor position to the screen.

Because the sensors provide high-resolution angle measurements and the mechanism has very little moving mass, the result is an extremely responsive pointing device well suited for rhythm games like osu!.

---

# Hardware

Required components:

| Quantity | Part                           |
| -------- | ------------------------------ |
| 1        | Arduino Pro Micro              |
| 2        | AS5600 Magnetic Rotary Encoder |
| 2        | F693ZZ 3x8x4mm 8mm Bearings    |
| 1        | 3.3V Buck Converter            |
| 2        | M3 Screws                      |
| 1        | 8mm (or smaller) Mouse Skate   |
| —        | Magnets - come with AS5600     |
| —        | 3D Printed Parts               |

---

# Wiring

## Encoder #1

| AS5600 | Pro Micro |
| ------ | --------- |
| SDA    | Pin 2     |
| SCL    | Pin 3     |

---

## Encoder #2

| AS5600 | Pro Micro |
| ------ | --------- |
| SDA    | Pin 4     |
| SCL    | Pin 5     |

---

## Power

```
Pro Micro VCC
        │
        ▼
Buck Converter VI

Buck Converter VO
        │
        ├────────► AS5600 #1 VCC
        └────────► AS5600 #2 VCC

All GND pins are connected together.
```
### **DIR and GND on the sensors MUST be connected**

The buck converter steps the Pro Micro's supply down to **3.3V** for both AS5600 sensors.

Place magnets which came with the AS5600s on the top of the bearings.

More in-depth instructions coming soon.

---

# Assembly

The entire frame is assembled using only **two M3 10mm screws**.

Install:

* both F693ZZ bearings
* the two AS5600 sensors (preferably a set with magnets)
* the printed arms
* the Pro Micro
* the 3.3V buck converter

For the pen holder, attach **a single dot mouse skate** to the bottom.

> **Important:** The mouse skate should be **8 mm or smaller**. 

---

# Software installation (Linux)

**Windows version a WIP**

Upload the Arduino firmware to the Pro Micro.

Open the driver folder and use commands:

```
python -m venv venv
pip install -r requirements.txt
```
Once connected over USB.

```
source venv/bin/activate.sh
python tabletto_driver.py
```
Ensure that your Arduino Pro Micro occupies port /dev/ttyACM0,

if not, either change that in the code (line 12) or try replugging the device.

To check what port your Arduino occupies you can use.

```
arduino-cli board list
```

---

# Calibration

Once the driver is running pull the arm straight to the right and press CTRL+2

Because this tablet doesn't know when the pen is lifted up you use CTRL+1 to enable/disable it

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

All .stl files can be found in the *models* directory

Every structural component is designed to be **3D printed**.

No CNC machining or laser cutting is required.

---

# License

Feel free to build, modify, and improve Tabletto. Contributions, bug reports, and pull requests are always welcome. 🎉
