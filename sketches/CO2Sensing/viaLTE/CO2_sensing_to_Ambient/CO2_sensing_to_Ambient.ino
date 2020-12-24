/*
 *  CO2_sensing.ino - CO2 Sensing for Emviroment to upload the Ambient via LTE.
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
#include <Flash.h>

#include <LowPower.h>
#include <RTC.h>

/***   Select Mode   ***/
#define USE_OLED
#define UPLOAD_AMBIENT

#ifdef UPLOAD_AMBIENT
#include <Arduino.h>
#include "Ambient_SpresenseLTEM.h"
LTEScanner theLteScanner;
#endif

#ifdef USE_OLED
#include "U8g2lib.h"
U8G2_SSD1327_EA_W128128_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#endif

#include "SparkFun_SCD30_Arduino_Library.h" 
SCD30 airSensor;

/**--- Definitions -------------------------------------*/
const int good_thr          = 1500; /* 2000ppm */
const int bad_thr           = 3500; /* 6000ppm */
const int reboot_thr        = 10;   /* 6000ppm */
const int setting_mode_time = 500;  /* 5000ms */

/**--- File name on FLASH ------------------------------*/
const char* apn_file  = "apn.txt";
const char* name_file = "name.txt";
const char* pass_file = "pass.txt";
const char* id_file   = "id.txt";
const char* key_file  = "key.txt";
const char* sens_file = "sens.txt";
const char* up_file   = "up.txt";

/**--- Setting Parameters ------------------------------*/
static String apnName = "xxxxxxxx";
static String usrName = "xxxxxxxx";
static String apnPass = "xxxxxxxx";

static uint16_t channelId = 00000;
static String writeKey  = "xxxxxxxxxxxxx";

/**--- Variable ----------------------------------------*/
static uint16_t sensing_interval = 0; /* n(s) */
static uint16_t upload_interval  = 0; /* 1/n */
//static uint16_t refresh_interval  = 10; /* 1/n */

static uint16_t error0 = 0; /* modem error */
static uint16_t error1 = 0; /* I2C error */
static uint16_t error2 = 0; /* No data */

#define STRING_BUFFER_SIZE 128
char StringBuffer[STRING_BUFFER_SIZE];

/**--- I2C adjust ---------------------------------------*/
#define IC_ENABLE   0x0418d46c
#define IC_SDA_HOLD  0x0418d47c
#define IC_SDA_SETUP 0x0418d494
#define SCU_CKEN    0x0410071c

void setup_i2c0_holdtime()
{
  uint32_t sdahold;
  uint32_t sdasetup;

  *(volatile uint32_t*)SCU_CKEN |= 2;

  uint32_t enable = *(volatile uint32_t*)IC_ENABLE;

  sdahold =  *(volatile uint32_t*)IC_SDA_HOLD;
  printf("IC_SDA_HOLD=0x%08x\n", sdahold);
  sdasetup =  *(volatile uint32_t*)IC_SDA_SETUP;
  printf("IC_SDA_SETUP=0x%08x\n", sdasetup);

  *(volatile uint32_t*)IC_ENABLE = enable & 0xfffffffe;

  *(volatile uint32_t*)IC_SDA_HOLD = 0x00050005;
  //*(volatile uint32_t*)IC_SDA_SETUP = 0x0;
  *(volatile uint32_t*)IC_ENABLE = enable | 1;

  sdahold =  *(volatile uint32_t*)IC_SDA_HOLD;
  printf("IC_SDA_HOLD=0x%08x\n", sdahold);
  sdasetup =  *(volatile uint32_t*)IC_SDA_SETUP;
  printf("IC_SDA_SETUP=0x%08x\n", sdasetup);

  printf("I2C0 hold time setting end\n"); 
}

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
  Serial.println("GOOD!");
  break;
  case E_WARN:
#ifdef USE_OLED
    u8g2.drawGlyph(8,16,64);
    u8g2.updateDisplayArea(0,0, 4, 2);      
#endif
  Serial.println("WARN!");
  break;
  case E_BAD:
#ifdef USE_OLED
    u8g2.drawGlyph(8,16,67);
    u8g2.updateDisplayArea(0,0, 4, 2);      
#endif
  Serial.println("BAD!");
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
  Serial.print("Please input Apn Name(");
  if(!settingString(apn_file, apnName)){ goto ERROR_EXIT; }

  Serial.print("Please input User Name(");
  if(!settingString(name_file, usrName)){ goto ERROR_EXIT; }
  
  Serial.print("Please input Apn Passphase(");
  if(!settingString(pass_file, apnPass)){ goto ERROR_EXIT; }
  
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

  if(!readString(apn_file, apnName)){ goto ERROR_RETURN; }
  if(!readString(name_file, usrName)){ goto ERROR_RETURN; }
  if(!readString(pass_file, apnPass)){ goto ERROR_RETURN; }
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
    u8g2.drawStr(20,40,"Please Setup");
    u8g2.drawStr(35,60,"via serial! ");
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
#ifdef USE_OLED
void drawBackgraund()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(20,40,"CO2(ppm):");
  u8g2.drawStr(24,56,"Temp(C):");
  u8g2.drawStr(24,72,"Humi(%):");
//  u8g2.drawStr(24,88,"I2C Error:");
  u8g2.sendBuffer();
}

void drawParameter(uint16_t co2, float temp, float humidity)
{
/*  u8g2.setDrawColor(1); 
  u8g2.drawBox(96,32,28,40);
  u8g2.setDrawColor(2); */

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(88,40,String(co2).c_str());
  u8g2.drawStr(88,56,String(temp).c_str());
  u8g2.drawStr(88,72,String(humidity).c_str());
/*  u8g2.updateDisplayArea(11,4, 4, 1); 
  u8g2.updateDisplayArea(11,6, 4, 1); 
  u8g2.updateDisplayArea(11,8, 4, 1);*/
  u8g2.sendBuffer();
}

void drawError()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(20,40,"I2C Error! ");
  u8g2.sendBuffer();
  sleep(1);
}

#endif

/**------------------------------------------*/
void setup()
{
  Serial.begin(115200);

  Wire.begin();
  setup_i2c0_holdtime();

  RTC.begin();
  LowPower.begin();

#ifdef USE_OLED
  u8g2.begin();
#endif

  menu();

  airSensor.begin();

  Serial.println("Device Initilizized.");

#ifdef UPLOAD_AMBIENT
  /* LTE Module Initialize */
  while(1) {
    Serial.println("Connecting Network...");
    if (theAmbient.begin(apnName ,usrName ,apnPass)) {
      break;
    }
    Serial.println("theAmbient begin failed");
    theAmbient.end();
    LowPower.deepSleep(3);
  }
  theAmbient.setupChannel(channelId, writeKey);
#endif

#ifdef USE_OLED
  drawBackgraund();
#endif
}

/**------------------------------------------*/
State theState;

void loop()
{
  static int counter = 0;
  static int counter_disp = 0;
  static int continuous_error = 0;
  static int co2 = 0;
  static float temp = 0;
  static float humi = 0;

  if(counter == 0){
    counter = upload_interval;
  }

  if(airSensor.dataAvailable()) {
    co2 = airSensor.getCO2();
    temp = airSensor.getTemperature();
    humi = airSensor.getHumidity();
    
    Serial.print("co2(ppm):");
    Serial.print(co2);

    Serial.print(" temp(C):");
    Serial.print(temp, 2);

    Serial.print(" humidity(%):");
    Serial.print(humi, 2);

    Serial.println();
    
    if(theState.update(co2)){
      theState.print();
    }

    continuous_error=0;

#ifdef USE_OLED
    drawParameter(co2,temp,humi);
#endif

  }else{
    continuous_error++;
    error2++;
    Serial.println("I2C error!");

#ifdef USE_OLED
//    drawError();
#endif

    if(continuous_error>reboot_thr){

#ifdef UPLOAD_AMBIENT
      theAmbient.end();
      sleep(1);
      return;
#endif
      LowPower.deepSleep(2);
    }
    airSensor.begin();
    sleep(1);
    return;
  }
  

  if(counter >= upload_interval){

#ifdef UPLOAD_AMBIENT
    puts("upload Ambient.");
    String strength = theLteScanner.getSignalStrength();
    Serial.print(strength);
    Serial.println(" [dBm]");

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(70,10,strength.c_str());
    u8g2.drawStr(90,10," [dBm]");
    u8g2.sendBuffer();

    theAmbient.set(1, (String(co2).c_str()));
    theAmbient.set(2, (String(temp).c_str()));
    theAmbient.set(3, (String(humi).c_str()));
    theAmbient.set(4, (String(error0).c_str()));
//    theAmbient.set(5, (String(error1).c_str()));
    theAmbient.set(6, (String(error2).c_str()));
    puts("Send Ambient.");
    int ret = theAmbient.send();

    if (ret == 0) {
      error0++;
      Serial.println("*** ERROR! LTE reboot! ***\n");
      theAmbient.end();
      while(1) {
        Serial.println("Connecting Network...");
        if (theAmbient.begin(apnName ,usrName ,apnPass)) {
          break;
        }
      }
      
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
