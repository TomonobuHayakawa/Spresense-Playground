/*
 *  Draw Processing from Spresense
 *  Copyright 2023 Tomonobu Hayakawa
 *
 *  Orignal from https://github.com/baggio63446333/SpresenseCameraApps/tree/main/UsbLiveStreaming
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

import java.util.Date;
import java.text.SimpleDateFormat;
import processing.serial.*;

Serial serial;

// Please change the serial setting for user environment
final String SERIAL_PORTNAME = "COM10";
final int    SERIAL_BAUDRATE = 921600;

void setup()
{
  serial = new Serial(this, SERIAL_PORTNAME, SERIAL_BAUDRATE);

  textSize(16);
}


// for debug
int count = 0;
int base_time = 0;

boolean find_sync(int timeout)
{
  String sync_words = "0000";
  int expire = millis() + timeout;

  while (true) {
    if (serial.available() > 0) {
      sync_words = sync_words.substring(1);
      sync_words = sync_words + (char)serial.read();
      if(sync_words.equals("SPRS")){
        return true;
      }
    } else {
      delay(50);
    }

    if (timeout > 0) {
      if (millis() > expire) {
        return false;
      }
    }
  }
}
  
void draw()
{
  int size = 0;

  // Search sync words
  if (find_sync(3000)) {
    // Receive a binary data size in 4byte
    size = serial.read()<<24 | serial.read()<<16 | serial.read()<<8 | serial.read();
  } else {
    println("recover1");
    recover();
    return;
  }

  // illegal size
  if (size <= 0) {
    serial.clear();
    return;
  }

  // for debug
  int now = millis();
  println(count++, ": size=", size, "time=", now - base_time, "[ms]");
  base_time = now;

  // Receive binary data
  byte [] data = new byte[size];
  int timeout = millis() + 5000;
  background(255);
  fill(0);
  for (int i = 0; i < size; ) {
    println("i=",i);
    if (serial.available() > 0) {
      data[i] = (byte)serial.read();
      println("data=",data[i]);
      text("data="+str(data[i]),10,20+i*15);
      i++;
      data[i] = (byte)serial.read();
      println("data=",data[i]);
      text("data="+str(data[i]),10,20+i*15);
      i++;

      float data_f = float(serial.read()<<8 | serial.read())/100;
      println("data_f=",data_f);
      text("data="+str(data_f),10,20+i*15);
      i=i+2;
      
    } else {
      if (millis() > timeout) {
        println("recover2");
        recover();
        return;
      }
      delay(50);
    }
  }
}

//
// recover any error
//
void recover()
{
  serial.clear();
  serial.stop();
  delay(1000);
  while (true) {
    serial = new Serial(this, SERIAL_PORTNAME, SERIAL_BAUDRATE);
    if (serial != null) {
      break;
    }
    delay(1000);
  }
}

String timestamp()
{
  Date date = new Date();
  SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMdd-HHmmss-SSS");
  return sdf.format(date);
}
