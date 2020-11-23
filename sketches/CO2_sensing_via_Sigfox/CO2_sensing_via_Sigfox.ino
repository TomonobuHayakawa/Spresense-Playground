/*
 *  CO2_sensing.ino - CO2 Sensing for Emviroment to upload the Cloud Services via Sigfox.
 *  Copyright 2020 Sony Semiconductor Solutions Corporation
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

#include <Arduino.h>
#include <Wire.h>
#include <File.h>

#include "SparkFun_SCD30_Arduino_Library.h" 
#include "U8g2lib.h"

#include "wf931.h"

SCD30 airSensor;
U8G2_SSD1327_EA_W128128_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);


#define STRING_BUFFER_SIZE 128
char StringBuffer[STRING_BUFFER_SIZE];

static int sensing_interval = 5; /* 5s */
static int upload_interval  = 6; /* 1/n */

static int setting_mode_time = 200; /* 2000ms */

/* Change the settings according to your sim card */
static String apnNanme;
static String apnUserNanme;
static String apnPasswd;

static uint16_t channelId;
static String writeKey;

void setup()
{
  Serial.println("Device Booting... ");

  Wire.begin();  
  Serial.begin(115200);
  u8g2.begin();

  Serial.println("Device Booted. ");
  
  Serial.println("Device Initilizing... ");

  airSensor.begin();

  Serial.println("Device Initilizized.");

  wf931.begin();
      
  sleep(1);
}

void drawBackgraund(uint16_t co2)
{
  u8g2.setFont(u8g2_font_open_iconic_weather_2x_t);
  if(co2<3000){
    u8g2.drawGlyph(10,20,69);
  }else if(co2<8000){
    u8g2.drawGlyph(10,20,64);
  }else{
    u8g2.drawGlyph(10,20,67);
  }
}

void drawParameter(uint16_t co2, float temp, float humidity)
{
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(20,40,"CO2(ppm):");
  u8g2.drawStr(85,40,String(co2).c_str());
  u8g2.drawStr(24,56,"Temp(è^\C):");
  u8g2.drawStr(85,56,String(temp).c_str());
  u8g2.drawStr(24,72,"Humi(%):");
  u8g2.drawStr(85,72,String(humidity).c_str());
}


void loop()
{
  static int counter = 0;

  if (airSensor.dataAvailable())
  {
    Serial.print("co2(ppm):");
    Serial.print(airSensor.getCO2());

    Serial.print(" temp(C):");
    Serial.print(airSensor.getTemperature(), 1);

    Serial.print(" humidity(%):");
    Serial.print(airSensor.getHumidity(), 1);

    Serial.println();
  } else {
    Serial.println("No data");
  }
  
  u8g2.clearBuffer();
  drawBackgraund(airSensor.getCO2());
  drawParameter(airSensor.getCO2(),airSensor.getTemperature(),airSensor.getHumidity());
  u8g2.sendBuffer();

  if(counter >= upload_interval){

    wf931.send(airSensor.getCO2(),
               (uint16_t)airSensor.getTemperature()*100,
               (uint16_t)airSensor.getHumidity()*100,
               0,
               0,
               0);
    counter = 0;
        
  }else{
    counter++;
  }

  sleep(sensing_interval);

}
