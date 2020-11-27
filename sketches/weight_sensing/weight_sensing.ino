/*
 *  Weight measurement sample by Spresense
 *  Copyright 2020 Tomonobu Hayakawa
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

/* You must change to the value of the individual sensor. */
#define A 000.00
#define B 0.000

/* You must change to the resistance value of your circuit. */
#define R 0000

const int amax = (1024*1.8)/0.7;

void setup() {
  Serial.begin(115200);          //  setup serial

}

void loop() {
  int val = analogRead(A3);    // read the input pin

  double y = (double)val*R/(amax-val);
  double x = pow((A/y),1/B);
  double g = x*1000/9.8;

  Serial.println(val);         // Analog value
  Serial.print(y);             // Resistance value
  Serial.println("(ohm)");
  Serial.print(x);             // N value
  Serial.println("(N)");       
  Serial.print(g);             // grams value
  Serial.println("(g)");
  Serial.print("\n");

  sleep(1);

}
