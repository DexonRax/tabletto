// Tabletto — dual-licensed under AGPLv3 for noncommercial use, or a separate
// commercial license for manufacturing/sale. See LICENSE and
// LICENSE-COMMERCIAL.md in the repo root, or contact the author.
//
// Copyright (C) 2026 Dexon Rax
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
 

// Bus A: hardware I2C (pins 2/3)
// Bus B: SoftWire (pins 4/5) - requires "SoftWire" library (Bill Greiman)
//   Library Manager -> search for "SoftWire"

#include <Wire.h>
#include <SoftWire.h>

#define AS5600_ADDR      0x36
#define REG_RAW_ANGLE_H  0x0C

// Bus B - software I2C pins
#define SOFT_SDA 4
#define SOFT_SCL 5

char swTxBuf[16];
char swRxBuf[16];
SoftWire swBus(SOFT_SDA, SOFT_SCL);

// Read RAW_ANGLE (12 bit) from hardware I2C (Bus A)
bool readHardware(uint16_t &value) {
  Wire.beginTransmission(AS5600_ADDR);
  Wire.write(REG_RAW_ANGLE_H);
  if (Wire.endTransmission(false) != 0) return false; // repeated start

  Wire.requestFrom((uint8_t)AS5600_ADDR, (uint8_t)2);
  if (Wire.available() < 2) return false;

  uint8_t hi = Wire.read();
  uint8_t lo = Wire.read();
  value = ((uint16_t)hi << 8 | lo) & 0x0FFF;
  return true;
}

// Read RAW_ANGLE (12 bit) from SoftWire (Bus B)
bool readSoft(uint16_t &value) {
  swBus.beginTransmission(AS5600_ADDR);
  swBus.write(REG_RAW_ANGLE_H);
  if (swBus.endTransmission(false) != 0) return false;

  swBus.requestFrom((uint8_t)AS5600_ADDR, (uint8_t)2);
  if (swBus.available() < 2) return false;

  uint8_t hi = swBus.read();
  uint8_t lo = swBus.read();
  value = ((uint16_t)hi << 8 | lo) & 0x0FFF;
  return true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  // Bus A - hardware
  Wire.begin();
  Wire.setClock(1000000);

  // Bus B - software
  swBus.setTxBuffer(swTxBuf, sizeof(swTxBuf));
  swBus.setRxBuffer(swRxBuf, sizeof(swRxBuf));
  swBus.setDelay_us(2);
  swBus.setTimeout(1000);
  swBus.begin();

  Serial.println("Start AS5600 x2 test");
}

void loop() {
  uint16_t angleA = 0, angleB = 0;
  bool okA = readHardware(angleA);
  bool okB = readSoft(angleB);

  uint16_t a = okA ? angleA : 0xFFFF;
  uint16_t b = okB ? angleB : 0xFFFF;

  uint8_t frame[5];
  frame[0] = 0xAA;
  frame[1] = a & 0xFF;
  frame[2] = (a >> 8) & 0xFF;
  frame[3] = b & 0xFF;
  frame[4] = (b >> 8) & 0xFF;
  Serial.write(frame, 5);
}
