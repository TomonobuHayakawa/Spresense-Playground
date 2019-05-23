/*
 *  wf931.ino - WF931(Sigfox Module) sample application
 *  Copyright 2018 Sony Semiconductor Solutions Corporation
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

bool result() {
  char c;
  do{
   c = Serial2.read();
   printf("%c", c);
   if(c==0xFF){
    printf("Error!");
    return false;
   }
  }while(c!='\r');    

  // for LF
  c = Serial2.read();
  printf("%c", c);

  return true;
}

void setup() {
  
  // Configure baud rate to 9600
  Serial2.begin(9600);

  // Configure reset pin
  pinMode(PIN_D26, OUTPUT);

  // Do reset
  digitalWrite(PIN_D26, LOW);
  delay(100);

  // Release reset
  digitalWrite(PIN_D26, HIGH);
  delay(100);
  result();

  // Get chip ID
  printf("ID = ");
  Serial2.write("AT$ID?\r");
  delay(100);
  result();

  // Send state by WF931
  Serial2.write("AT$SB=1\r");
  printf("Send State = 1\n");
  sleep(10);
  result();
  
}

void loop() {

  puts("loop!\n");

  static char i = 0;
  // wakeup.
  Serial2.write("0x00");
  delay(60);

  // Sensing, etc...

   i = (i+1) % 7;
   char c = '0'+i;
  
  // Send Data
  Serial2.write("AT$SF=");
  Serial2.write(c);
  Serial2.write(c+1);
  Serial2.write(c+2);
  Serial2.write(c+3);
  Serial2.write("\r");
  
  printf("SendDate = %c%c%c%c\n",c,c+1,c+2,c+3);
  sleep(10);
  result();
  
  sleep(1000);
}
