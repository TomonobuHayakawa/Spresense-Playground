/*
 *  CO2_sensing.ino - CO2 Sensing for Emviroment to upload Machinist provide by IIJ via LTE.
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
#define UPLOAD_MACHINIST

#ifdef UPLOAD_MACHINIST
#include <LTE.h>
#include "IIJMachinistClient.h"
LTEScanner theLteScanner;
LTE lteAccess;
IIJMachinistClient* theMachinistClient;
#endif

#ifdef USE_OLED
#include "U8g2lib.h"
U8G2_SSD1327_EA_W128128_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#endif

#include "SparkFun_SCD30_Arduino_Library.h" 
SCD30 airSensor;

/**--- File name on FLASH ------------------------------*/
const char* apn_file  = "apn.txt";
const char* name_file = "name.txt";
const char* pass_file = "pass.txt";
const char* akey_file = "akey.txt";
const char* sens_file = "sens.txt";
const char* up_file   = "up.txt";

/*const char* agent_file = "agent.txt";
const char* space_file = "space.txt";*/

/**--- Definitions -------------------------------------*/
const int good_thr          = 1500; /* 2000ppm */
const int bad_thr           = 3500; /* 6000ppm */
const int reboot_thr        = 10;   /* 6000ppm */
const int setting_mode_time = 500;  /* 5000ms */

#define AGENT      "Spresense-demo0"
#define NAMESPACE  "demo0"
#define NAME0      "CO2"
#define NAME1      "Temp"
#define NAME2      "Humm"
#define NAME3      "NetErr"
#define NAME4      "I2cErr"
#define KEY        "place"
#define VALUE      "tokyo"

/**--- Setting Parameters ------------------------------*/
static String apnName = "xxxxxxxx";
static String usrName = "xxxxxxxx";
static String apnPass = "xxxxxxxx";

static String apiKey  = "xxxxxxxxxxxxx";

/**--- Variable ----------------------------------------*/
static uint16_t sensing_interval = 0; /* 5s */
static uint16_t upload_interval  = 0; /* 1/n */

static uint16_t error0 = 0; /* modem error */
static uint16_t error1 = 0; /* I2C error */

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
    
  Serial.print("Please input API Key(");
  if(!settingString(akey_file, apiKey)){ goto ERROR_EXIT; }

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
  if(!readString(akey_file, apiKey)){ goto ERROR_RETURN; }
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
void setup()
{
  Serial.begin(115200);

  Wire.begin();

  RTC.begin();
  LowPower.begin();

#ifdef USE_OLED
  u8g2.begin();
#endif

  menu();

  airSensor.begin();

  Serial.println("Device Initilizized.");

#ifdef UPLOAD_MACHINIST

  theMachinistClient = new IIJMachinistClient(apiKey);

  while (true) {
    if (lteAccess.begin() == LTE_SEARCHING) {
      if (lteAccess.attach(apnName.c_str(), usrName.c_str(), apnPass.c_str()) == LTE_READY) {
        Serial.println("attach succeeded.");
        break;
      }
      Serial.println("An error occurred, shutdown and try again.");
      lteAccess.shutdown();
      sleep(1);
    }
  }

  theMachinistClient->setDebugSerial(Serial);
  theMachinistClient->init();
#endif

}

/**------------------------------------------*/
#ifdef USE_OLED
void drawBackgraund()
{
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(20,40,"CO2(ppm):");
  u8g2.drawStr(24,56,"Temp(C):");
  u8g2.drawStr(24,72,"Humi(%):");
//  u8g2.drawStr(24,88,"I2C Error:");
}

void drawParameter(uint16_t co2, float temp, float humi)
{
/*  u8g2.setDrawColor(1); 
  u8g2.drawBox(96,32,28,40);
  u8g2.setDrawColor(2); */

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(88,40,String(co2).c_str());
  u8g2.drawStr(88,56,String(temp).c_str());
  u8g2.drawStr(88,72,String(humi).c_str());
/*  u8g2.updateDisplayArea(11,4, 4, 1); 
  u8g2.updateDisplayArea(11,6, 4, 1); 
  u8g2.updateDisplayArea(11,8, 4, 1);*/
//  u8g2.sendBuffer();
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

State theState;

void loop()
{
  static int co2 = 0;
  static float temp = 0;
  static float humi = 0;
  static int counter = 0;
  static int continuous_error = 0;
  int ret;

  if(counter == 0){
    counter = upload_interval;
  }

  if(airSensor.dataAvailable()) {
    co2 = airSensor.getCO2();
    temp = airSensor.getTemperature();
    humi = airSensor.getHumidity();

    theState.update(co2);
    
    Serial.print("co2(ppm):");
    Serial.print(co2);

    Serial.print(" temp(C):");
    Serial.print(temp, 2);

    Serial.print(" humidity(%):");
    Serial.print(humi, 2);

    Serial.println();
    
    continuous_error=0;

#ifdef USE_OLED
    u8g2.clearBuffer();
    drawBackgraund();
    theState.print();
    drawParameter(co2,temp,humi);
    u8g2.sendBuffer();
#endif

  }else{
    continuous_error++;
    error1++;
    Serial.println("I2C error!");

#ifdef USE_OLED
    drawError();
#endif

    if(continuous_error>reboot_thr){

#ifdef UPLOAD_AMBIENT
      lteAccess.shutdown();
      sleep(1);
#endif
      LowPower.deepSleep(2);
    }
    sleep(1);
    airSensor.begin();
    sleep(1);
    return;
  }

  if(counter >= upload_interval){
#ifdef UPLOAD_MACHINIST
    String strength = theLteScanner.getSignalStrength();
    Serial.print(strength);
    Serial.println(" [dBm]");
#ifdef USE_OLED
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(70,10,strength.c_str());
    u8g2.drawStr(90,10," [dBm]");
    u8g2.sendBuffer();
#endif

    ret = theMachinistClient->post(AGENT, NAMESPACE, NAME0, co2, KEY, VALUE);
    if(ret!=200){ goto ERROR_RETURN; }
    ret = theMachinistClient->post(AGENT, NAMESPACE, NAME1, temp, KEY, VALUE);
    if(ret!=200){ goto ERROR_RETURN; }
    ret = theMachinistClient->post(AGENT, NAMESPACE, NAME2, humi, KEY, VALUE);
    if(ret!=200){ goto ERROR_RETURN; }
/*    ret = theMachinistClient->post(AGENT, NAMESPACE, NAME3, error0, KEY, VALUE);
    if(ret!=200){ goto ERROR_RETURN; }
    ret = theMachinistClient->post(AGENT, NAMESPACE, NAME4, error1, KEY, VALUE);
    if(ret!=200){ goto ERROR_RETURN; }*/

#endif
    Serial.println("*** Send comleted! ***\n");
    counter = 1;
      
  }else{
    counter++;
  }

  sleep(sensing_interval);
  return;

#ifdef UPLOAD_MACHINIST

ERROR_RETURN:
  Serial.println("NG status=" + String(ret));
  while (true) {
    lteAccess.shutdown();
    if (lteAccess.begin() == LTE_SEARCHING) {
      if (lteAccess.attach(apnName.c_str(), usrName.c_str(), apnPass.c_str()) == LTE_READY) {
        Serial.println("attach succeeded.");
        break;
      }
      Serial.println("An error occurred, shutdown and try again.");
      lteAccess.shutdown();
      sleep(1);
    }
  }

  return;

#endif

}
