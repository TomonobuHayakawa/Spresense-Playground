/*
 *  ControllerCore.ino - Interface Controller(Switch/RotaryEncoder/etc..) on SpreSynth
 *  Copyright 2023 T.Hayakawa
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
 
#include <MP.h>

#include <Wire.h>
#include <CY8C95X0.h>
#include <CY8C95X0_BASE.h>
#include "RotaryEncoder.h"

#include <RTC.h>

#if (SUBCORE != 1)
#error "Core selection is wrong!!"
#endif

CY8C95X0* gpio_ex;

class RotarySwitch{
public:
  RotarySwitch(pin_t _pin0, pin_t _pin1):pin0(_pin0),pin1(_pin1){}

  void init(int base){
    gpio_ex->_pinMode(pin0, INPUT);
    gpio_ex->_driveSelectPin(pin0, DRIVE_OPENHIGH);
    gpio_ex->_pinMode(pin1, INPUT);
    gpio_ex->_driveSelectPin(pin1, DRIVE_OPENHIGH);
    Enc.init(base);
  }

  bool set(){
//    return Enc.set(read(pin0),read(pin1));
    return Enc.set(gpio_ex->_digitalRead(pin0),gpio_ex->_digitalRead(pin1));
  }
  
  int8_t get(){
    return Enc.get();
  }

private:

  boolean read(pin_t pin){
    boolean data0 = gpio_ex->_digitalRead(pin);
    while(1){
      boolean data1 = gpio_ex->_digitalRead(pin);
      if(data0 == data1) return data0;
      data1 = data0;
      puts("miss");
    }
  }

  pin_t pin0;
  pin_t pin1;
  RotaryEncoder Enc;

};

class pushSwitch{
public:

  pushSwitch(pin_t in, pin_t out):sw_pin(in),led_pin(out){}

  void init(){
    state = false;
    push_time = RTC.getTime();
    gpio_ex->_pinMode(sw_pin, INPUT);
    gpio_ex->_driveSelectPin(sw_pin, DRIVE_OPENHIGH);
    gpio_ex->_pinMode(led_pin, OUTPUT);
    gpio_ex->_driveSelectPin(led_pin, DRIVE_OPENHIGH);
    gpio_ex->_digitalWrite(led_pin,state);
  }

  bool read(){
    if( !gpio_ex->_digitalRead(sw_pin) ) {
      RtcTime now = RTC.getTime();
      if((now-push_time) > 1){
        state = state ? false : true;
        push_time = now;
        gpio_ex->_digitalWrite(led_pin,state);
        return true;
      }
    }
    return false;
  }

  bool get(){ return state; }

private:
  boolean state;
  pin_t   sw_pin;
  pin_t   led_pin;
  RtcTime push_time;


};

RotarySwitch RotarySwitches[8] = {
  {{PIN_G0,PIN_3},{PIN_G0,PIN_4}},
  {{PIN_G0,PIN_5},{PIN_G0,PIN_6}},
  {{PIN_G0,PIN_7},{PIN_G3,PIN_0}},
  {{PIN_G3,PIN_1},{PIN_G3,PIN_2}},
  {{PIN_G3,PIN_3},{PIN_G3,PIN_4}},
  {{PIN_G3,PIN_5},{PIN_G3,PIN_6}},
  {{PIN_G3,PIN_7},{PIN_G5,PIN_7}},
  {{PIN_G5,PIN_6},{PIN_G5,PIN_2}}
};

pushSwitch pushSwitches[8] = {
  {{PIN_G4,PIN_0},{PIN_G6,PIN_3}},
  {{PIN_G4,PIN_1},{PIN_G6,PIN_2}},
  {{PIN_G4,PIN_2},{PIN_G6,PIN_1}},
  {{PIN_G4,PIN_3},{PIN_G6,PIN_0}},
  {{PIN_G5,PIN_5},{PIN_G4,PIN_4}},
  {{PIN_G5,PIN_4},{PIN_G4,PIN_5}},
  {{PIN_G5,PIN_0},{PIN_G4,PIN_6}},
  {{PIN_G5,PIN_1},{PIN_G4,PIN_7}}
};

class PushRotarySwitch{
public:
  PushRotarySwitch(pin_t _pin0, pin_t _pin1, pin_t _sw_pin):pin0(_pin0),pin1(_pin1),sw_pin(_sw_pin){}

  void init(int base){
    push_time = RTC.getTime();
    gpio_ex->_pinMode(pin0, INPUT);
    gpio_ex->_driveSelectPin(pin0, DRIVE_OPENHIGH);
    gpio_ex->_pinMode(pin1, INPUT);
    gpio_ex->_driveSelectPin(pin1, DRIVE_OPENHIGH);
    gpio_ex->_pinMode(sw_pin, INPUT);
    gpio_ex->_driveSelectPin(sw_pin, DRIVE_OPENHIGH);
    Enc.init(base);
  }

  bool set(){
//    return Enc.set(read(pin0),read(pin1));
    return Enc.set(gpio_ex->_digitalRead(pin0),gpio_ex->_digitalRead(pin1));
  }
  
  int8_t get(){
    return Enc.get();
  }

  bool read(){
    if( !gpio_ex->_digitalRead(sw_pin) ) {
      RtcTime now = RTC.getTime();
      if((now-push_time) > 1){
        push_time = now;
        return true;
      }
    }
    return false;
  }

private:

  RtcTime push_time;
  pin_t pin0;
  pin_t pin1;
  pin_t sw_pin;
  RotaryEncoder Enc;

};

PushRotarySwitch PushRotarySwitches[3] = {
  {{PIN_G7,PIN_7},{PIN_G7,PIN_6},{PIN_G5,PIN_3}},
  {{PIN_G7,PIN_4},{PIN_G7,PIN_3},{PIN_G7,PIN_5}},
  {{PIN_G7,PIN_0},{PIN_G7,PIN_1},{PIN_G7,PIN_2}}
};

void setup()
{
  pinMode(19, OUTPUT);  
  digitalWrite(19, HIGH);

  RTC.begin();
  MP.begin();
  MP.RecvTimeout(MP_RECV_POLLING);

  digitalWrite(19, LOW);
  
  Serial.begin(115200);
  Wire.begin();
  gpio_ex = new CY8C95X0(60); 
  puts("Core1 booted.");
  
  gpio_ex->pinMode(PIN_G5,PIN_3,INPUT);

  for(int i=0;i<8;i++){
    pushSwitches[i].init();
  }

  for(int i=0;i<4;i++){
    RotarySwitches[i].init(48);
  }
  
  for(int i=4;i<8;i++){
    RotarySwitches[i].init(60);
  }

  PushRotarySwitches[0].init(-24);
  PushRotarySwitches[1].init(50);
  PushRotarySwitches[2].init(0);
}

void loop()
{
  for(int i=0;i<8;i++){
    if(!pushSwitches[i].get()) continue; 
    if(RotarySwitches[i].set()){
      int ret = MP.Send((10+i), RotarySwitches[i].get(),0);
      if (ret < 0) puts("error! 0");
      ret = MP.Send((10+i), RotarySwitches[i].get(),2);
      if (ret < 0) puts("error! 1");
      
/*      Serial.print("Enc");
      Serial.print(i);
      Serial.print(":");
      Serial.println(RotarySwitches[i].get());*/
    }
  }

  for(int i=0;i<8;i++){
    if(pushSwitches[i].read()){
      int ret = MP.Send((20+i), pushSwitches[i].get(),0);
      if (ret < 0) puts("error! 0");
      ret = MP.Send((20+i), pushSwitches[i].get(),2);
      if (ret < 0) puts("error! 2");

/*      if(pushSwitches[i].get()){
        printf("SW%d On\n",i);
      }else{
        printf("SW%d Off\n",i);
      }*/
    }
  }

  for(int i=1;i<2;i++){
    if(PushRotarySwitches[i].set()){
      int ret = MP.Send((30+i), PushRotarySwitches[i].get(),0);
      if (ret < 0) puts("error!");
      ret = MP.Send((30+i), PushRotarySwitches[i].get(),2);
      if (ret < 0) puts("error!");
      
/*      Serial.print("Enc");
      Serial.print(i);
      Serial.print(":");
      Serial.println(RotarySwitches[i].get());*/
    }
  }

  for(int i=2;i<3;i++){
    if(PushRotarySwitches[i].read()){
      int ret = MP.Send((40+i), 1, 0);
      if (ret < 0) puts("error!");
      ret = MP.Send((40+i), 1, 2);
      if (ret < 0) puts("error!");
      
/*      Serial.print("Enc");
      Serial.print(i);
      Serial.print(":");
      Serial.println(RotarySwitches[i].get());*/
    }
  }


}
