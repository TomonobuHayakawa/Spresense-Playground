/*
 *  SensorCore.ino - Impact detection with ADXL335 on SensorCore.
 *  Copyright 2026 T.Hayakawa
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#if (SUBCORE != 1)
#error "Core selection is wrong!!"
#endif

#include <MP.h>
#include <ADXL335.h>

static constexpr int PIN_X = A0;
static constexpr int PIN_Y = A1;
static constexpr int PIN_Z = A2;

static constexpr int CENTER_X = 230;
static constexpr int CENTER_Y = 230;
static constexpr int CENTER_Z = 230;
static constexpr int ONE_G_X = 270;
static constexpr int ONE_G_Y = 270;
static constexpr int ONE_G_Z = 270;

static constexpr float SHOCK_THRESHOLD_G = 1.14f;
// Ignore new detections for this duration after one hit.
static constexpr unsigned long DETECTED_TIME = 100UL;

ADXL335 accel;
static unsigned long last_detect_time = 0;

void setup()
{
  int ret = MP.begin();
  if (ret < 0) {
    errorLoop(2);
  }

  accel.begin(PIN_X, PIN_Y, PIN_Z);
  accel.setCalibration(CENTER_X, ONE_G_X,
                       CENTER_Y, ONE_G_Y,
                       CENTER_Z, ONE_G_Z);
  accel.setShockThreshold(SHOCK_THRESHOLD_G);
}

bool detect_impact_event()
{
  unsigned long now = millis();
  float magnitude, ax, ay, az;

  if ((now - last_detect_time) < DETECTED_TIME) {
    return false;
  }

  if (accel.detectShock(magnitude, ax, ay, az)) {
    last_detect_time = now;
    ledOn(LED0);
    return true;
  }

  return false;
}

void loop()
{
  unsigned long now = millis();
  if ((now - last_detect_time) >= DETECTED_TIME) {
    ledOff(LED0);
  }

  if (detect_impact_event()) {
    int ret;
    int8_t msgid = 10;
    uint32_t msgdata = 1;

    ret = MP.Send(msgid, msgdata);
    if (ret < 0) {
      errorLoop(4);
    }
  }

  delay(10);
}

void errorLoop(int num)
{
  while (1) {
    for (int i = 0; i < num; i++) {
      ledOn(LED0);
      delay(300);
      ledOff(LED0);
      delay(300);
    }
    delay(1000);
  }
}