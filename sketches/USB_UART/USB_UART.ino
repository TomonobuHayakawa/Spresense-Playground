/*
    USB_UART.ino - USB <-> UART sample
    Copyright 2019 Sony Semiconductor Solutions Corporation

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

void setup()
{
  Serial.begin(921600);
//  Serial.begin(115200);
  while (!Serial);
  Serial2.begin(921600);
//   Serial2.begin(115200);
  while (!Serial2);
}

void loop()
{
  while (Serial.available()) {
//    ledOn(LED0);
    Serial2.write(Serial.read());
//    ledOff(LED0);
  }
  while (Serial2.available()) {
//    ledOn(LED1);
    Serial.write(Serial2.read());
//    ledOff(LED1);
  }

}
