/*
 *  pickup.ino - CO-(Robot ARM) for Spresense example (pickup & release objects)
 *  Copyright AUTOLAB & T.Hayakawa
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

/*****************************************************
ピンの接続　pca9685側　 
OE->フリー(内部でプルダウンされている)
全体の出力カットに利用

servo の番号
0　チャック　開閉
1　胴体上側　上下
2　胴体下側　上下
3　ベース　　左右

*****************************************************/

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define CHUCK  0
#define UPPER  1
#define LOWER  2
#define BASE   3

#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates
#define NumServo_Max 4 //4 servos connetcting

bool move_joint(uint8_t joint, int degrees)
{
  if(joint>=NumServo_Max) return false;

  //上記2つの変数は指定したい角度*10+600を
  //pwm.writeMicroseconds(servonum,○○)にいれる為のものです
  //　例）600 //0°に指定、
  //　　　1500 //90°に指定（ロボットは90°が正面になるように作られています）
  //　　　2400 //180°に指定
  pwm.writeMicroseconds(joint,(degrees*10+600));
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Setup Servo!");
  pwm.begin();

  // In theory the internal oscillator is 25MHz but it really isn't
  // that precise. You can 'calibrate' by tweaking this number till
  // you get the frequency you're expecting!
  pwm.setOscillatorFrequency(27000000);  // The int.osc. is closer to 27MHz
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  delay(10);

  /* Initial Posture*/
  pwm.writeMicroseconds(CHUCK,50);
  pwm.writeMicroseconds(UPPER,110);
  pwm.writeMicroseconds(LOWER,90);
  pwm.writeMicroseconds(BASE ,90);
  delay(2);

}

void loop() {
  Serial.println("Pickup Start");

  move_joint(UPPER,30);
  move_joint(LOWER,19);
  sleep(2);

  move_joint(CHUCK,90);
  sleep(2);

  move_joint(UPPER,110);
  move_joint(LOWER,90);
  sleep(2);

  move_joint(BASE ,150);
  sleep(2);

  move_joint(UPPER,30);
  move_joint(LOWER,20);
  sleep(2);

  move_joint(CHUCK,50);
  sleep(2);

  move_joint(UPPER,110);
  move_joint(LOWER,90);
  sleep(2);

  move_joint(BASE ,90);

  Serial.println("Pickup End");
  sleep(10);

}
