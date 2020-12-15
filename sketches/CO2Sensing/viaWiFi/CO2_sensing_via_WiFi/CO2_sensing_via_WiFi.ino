/*
 *  CO2_sensing_via_WiFi.ino - CO2 Sensing for Emviroment to upload the Ambient via GS2200.
 *  Copyright 2020 Sresense Users
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
 
#include "AmbientGs2200.h"

#include <Arduino.h>
#include <Wire.h>
#include <File.h>
#include <Flash.h>

#include <LowPower.h>
#include <RTC.h>

/***   Select Mode   ***/
#define USE_OLED
#define UPLOAD_AMBIENT

#include "SparkFun_SCD30_Arduino_Library.h" 

#ifdef UPLOAD_AMBIENT
#include <GS2200Hal.h>
#include <GS2200AtCmd.h>
#include "AppFunc.h"
#include "AmbientGs2200.h"
#endif

#ifdef USE_OLED
#include "U8g2lib.h"
U8G2_SSD1327_EA_W128128_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#endif

SCD30 airSensor;

/**--- Definitions -------------------------------------*/
const int good_thr          = 1500; /* 2000ppm */
const int bad_thr           = 3500; /* 6000ppm */
const int reboot_thr        = 10;   /* 6000ppm */
const int setting_mode_time = 500;  /* 5000ms */

/**--- File name on FLASH ------------------------------*/
const char* apn_file  = "apn.txt";
const char* pass_file = "pass.txt";
const char* id_file   = "id.txt";
const char* key_file  = "key.txt";
const char* sens_file = "sens.txt";
const char* up_file   = "up.txt";

/**--- Setting Parameters ------------------------------*/
static String apSsid = "xxxxxxxx";
static String apPass = "xxxxxxxx";

static uint16_t channelId = 00000;
static String writeKey  = "xxxxxxxxxxxxx";

/**--- Variable ----------------------------------------*/
static uint16_t sensing_interval = 0; /* n(s) */
static uint16_t upload_interval  = 0; /* 1/n */

static uint16_t error0 = 0; /* modem error */
static uint16_t error1 = 0; /* I2C error */
static uint16_t error2 = 0; /* No data */

/**--- State class --------------------------------------*/
enum e_STATE {
  E_ERROR =0,
  E_GOOD,
  E_WARN,
  E_BAD
};

class State{

public:
  State():m_cur(E_ERROR){}
  void print();
  bool update(uint16_t);
  void clear(){m_cur=E_ERROR;}
  e_STATE get(){return m_cur;}
private:
  e_STATE m_cur;
};

void State::print()
{
#ifdef USE_OLED
  u8g2.setFont(u8g2_font_open_iconic_weather_2x_t);
#endif

  switch (m_cur){
  case E_GOOD:
#ifdef USE_OLED
    u8g2.drawGlyph(8,16,69);
    u8g2.updateDisplayArea(0,0, 4, 2);
#endif
//  Serial.println("GOOD!");
  break;
  case E_WARN:
#ifdef USE_OLED
    u8g2.drawGlyph(8,16,67);
    u8g2.updateDisplayArea(0,0, 4, 2);      
#endif
//  Serial.println("WARN!");
  break;
  case E_BAD:
#ifdef USE_OLED
    u8g2.drawGlyph(8,16,64);
    u8g2.updateDisplayArea(0,0, 4, 2);      
#endif
//  Serial.println("BAD!");
  break;
  default:
  break; 
  }
}

bool State::update(uint16_t co2)
{
  e_STATE next;
  if(co2<good_thr){
    next = E_GOOD;
  }else if(co2<bad_thr){
    next = E_WARN;
  }else{
    next = E_BAD;
  }

  if(m_cur==next) return false;

  m_cur = next;
  return true;
}

/**------------------------------------------*/
bool settingUint16(const char* name, uint16_t& data)
{
  Serial.print(data);
  Serial.println(") : ");
  while(!Serial.available()); 
  String tmp = Serial.readString();
  if(tmp.length() > 1){
    Flash.remove(name);
    File settingFile = Flash.open(name, FILE_WRITE);
    if(settingFile==0){    
     return false;
    }
    data = tmp.toInt();
    settingFile.write(tmp.c_str());
    settingFile.close();
  }
  Serial.println(data);
  return true;
}

bool settingString(const char* name, String& data)
{
  Serial.print(data);
  Serial.println(") : ");
  while(!Serial.available()); 
  String tmp = Serial.readString();
  if(tmp.length() > 1){
    Flash.remove(name);
    File settingFile = Flash.open(name, FILE_WRITE);
    if(settingFile==0){    
     return false;
    }
    tmp.trim();
    data = tmp;
    settingFile.write(data.c_str());
    settingFile.close();
  }
  Serial.println(data);
  return true;
}

void setting()
{
  Serial.print("Please input AP SSID (");
  if(!settingString(apn_file, apSsid)){ goto ERROR_EXIT; }

  Serial.print("Please input AP Passphase(");
  if(!settingString(pass_file, apPass)){ goto ERROR_EXIT; }
  
  Serial.print("Please input Ambient Channel ID(");
  if(!settingUint16(id_file, channelId)){ goto ERROR_EXIT; }
  
  Serial.print("Please input Ambient writeKey(");
  if(!settingString(key_file, writeKey)){ goto ERROR_EXIT; }

  Serial.print("Please input Sensing Interval(sec) (");
  if(!settingUint16(sens_file, sensing_interval)){ goto ERROR_EXIT; }

  Serial.print("Please input Sensing Interval(1/n) (");
  if(!settingUint16(up_file, upload_interval)){ goto ERROR_EXIT; }

  return;

ERROR_EXIT:
  Serial.println("Setting Error!");
  exit(1);
}


/**------------------------------------------*/
bool readUint16(const char* name, uint16_t& data)
{
  Serial.println(name);
  File settingFile = Flash.open(name, FILE_READ);
  if(settingFile.available()>0) {
    data = settingFile.readString().toInt();
    Serial.println(data);
  }else{
    return false;
  }
  settingFile.close();
  return true;
}

bool readString(const char* name, String& data)
{
  Serial.println(name);
  File settingFile = Flash.open(name, FILE_READ);
  if(settingFile.available()>0) {
    data = settingFile.readString();
    Serial.println(data);
  }else{
    return false;
  }
  settingFile.close();
  return true;
}


bool drawSettingMode()
{
#ifdef USE_OLED
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(10,40,"Device Booting... ");
  u8g2.sendBuffer(); 
#endif

  if(!readString(apn_file, apSsid)){ goto ERROR_RETURN; }
  if(!readString(pass_file, apPass)){ goto ERROR_RETURN; }
  if(!readUint16(id_file, channelId)){ goto ERROR_RETURN; }
  if(!readString(key_file, writeKey)){ goto ERROR_RETURN; }
  if(!readUint16(sens_file, sensing_interval)){ goto ERROR_RETURN; }
  if(!readUint16(up_file, upload_interval)){ goto ERROR_RETURN; }

  return true;

ERROR_RETURN:
  Serial.println("Setting Error!");
  return false;
}


void menu()
{
  if(!drawSettingMode()) {

#ifdef USE_OLED
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(20,40,"Plase Setup via serial! ");
    u8g2.sendBuffer();
#endif
    setting();
  }

  bootcause_e bc = LowPower.bootCause();

  if ((bc == POR_SUPPLY) || (bc == POR_NORMAL)) {

    Serial.println("Please push \'s\' to Setting menu.");
    for(int i=0;i<setting_mode_time;i++){
      if(Serial.available()){
        if(Serial.read()=='s'){
          Serial.read(); // remove '\n'
          setting();
          break;
        }
      }
      usleep(10000); // 10ms
    }
  }
  Serial.println("Nomal mode start!.");
}


/**------------------------------------------*/
void setup()
{
  Wire.begin();
  Serial.begin(115200);

  RTC.begin();
  LowPower.begin();

#ifdef USE_OLED
  u8g2.begin();
#endif

  menu();

  airSensor.begin();

#ifdef UPLOAD_AMBIENT
  /* WiFi Module Initialize */
  Init_GS2200_SPI();
  AtCmd_Init();

  App_InitModule(apSsid,apPass);
  App_ConnectAP();

  AmbientGs2200.begin(channelId, writeKey.c_str());
#endif
  
  sleep(1);
}

/**------------------------------------------*/
#ifdef USE_OLED
void drawParameter(uint16_t co2, float temp, float humidity)
{
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(20,40,"CO2(ppm):");
  u8g2.drawStr(85,40,String(co2).c_str());
  u8g2.drawStr(24,56,"Temp(C):");
  u8g2.drawStr(85,56,String(temp).c_str());
  u8g2.drawStr(24,72,"Humi(%):");
  u8g2.drawStr(85,72,String(humidity).c_str());
}
#endif

/**------------------------------------------*/
State theState;

void loop()
{
  static int counter = 0;
  static int continuous_error = 0;
  static int co2 = 0;
  static float temp = 0;
  static float humi = 0;

  if(counter == 0){
    counter = upload_interval;
  }

  if (airSensor.dataAvailable())
  {
    co2 = airSensor.getCO2();
    temp = airSensor.getTemperature();
    humi = airSensor.getHumidity();
    
    Serial.print("co2(ppm):");
    Serial.print(co2);

    Serial.print(" temp(C):");
    Serial.print(temp, 2);

    Serial.print(" humidity(%):");
    Serial.println(humi, 2);
 
  } else {
    error2++;
    continuous_error++;
    Serial.println("I2C error!");

#ifdef USE_OLED
//    drawError();
#endif

    if(continuous_error>reboot_thr){
      LowPower.deepSleep(2);
    }
    airSensor.begin();
    sleep(1);
    return;
  }
  
#ifdef USE_OLED
  u8g2.clearBuffer();
#endif

  if(theState.update(co2)){
    theState.print();
  }

#ifdef USE_OLED
  drawParameter(co2,temp,humi);
  u8g2.sendBuffer();
#endif

  if(counter >= upload_interval){
#ifdef UPLOAD_AMBIENT
    AmbientGs2200.set(1, (String(co2).c_str()));
    AmbientGs2200.set(2, (String(temp).c_str()));
    AmbientGs2200.set(3, (String(humi).c_str()));
    AmbientGs2200.set(4, (String(error0).c_str()));
//    AmbientGs2200.set(5, (String(error1).c_str()));
    AmbientGs2200.set(6, (String(error2).c_str()));
    int ret = AmbientGs2200.send();

    if (ret == 0) {
      Serial.println("*** ERROR! RESET Wifi! ***\n");
      App_InitModule(apSsid,apPass);
      App_ConnectAP();
    }else{
      Serial.println("*** Send comleted! ***\n");
      counter = 1;
    }
#else
    counter = 1;
#endif

  }else{
    counter++;
  }

  sleep(sensing_interval);

}
