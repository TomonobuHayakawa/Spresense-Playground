/*
 *  VoicelControl.ino - CO-(Robot ARM) for Spresense example (Control by voice)
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

#include <SDHCI.h>
#include <FrontEnd.h>
#include <Recognizer.h>
#include <MemoryUtil.h>
#include <EEPROM.h>
#include <stdio.h>
#include <stdlib.h>

SDClass theSD;
FrontEnd *theFrontend;
Recognizer *theRecognizer;

#define ANALOG_MIC_GAIN  180 /* +18dB */
static const uint8_t  recognizer_channel_number = 1;
static const uint8_t  recognizer_bit_length = 16;
uint8_t g_event_char;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define CHUCK  0
#define UPPER  1
#define LOWER  2
#define BASE   3

#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates
#define NumServo_Max 4 //4 servos connetcting

/* Initial Positions */
#define INI_CHUCK_POS  50
#define INI_UPPER_POS  110
#define INI_LOWER_POS  90
#define INI_BASE_POS   90

static void menu()
{
  printf("=== MENU (input voice command! ) ==============\n");
  printf("saisei kaishi:  play  \n");
  printf("ichiji teishi:  stop  \n");
  printf("mae ni tobu:    next  \n");
  printf("ushiro ni tobu: prev  \n");
  printf("=====================================\n");
}

bool pos_joint(uint8_t joint, int degrees)
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

/**
 * @brief FrontEnd done callback procedure
 *
 * @param [in] event        AsRecorderEvent type indicator
 * @param [in] result       Result
 * @param [in] sub_result   Sub result
 *
 * @return true on success, false otherwise
 */
static bool frontend_done_callback(AsMicFrontendEvent ev, uint32_t result, uint32_t sub_result)
{
  printf("frontend_done_callback cb\n");
  return true;
}

static void recognizer_done_callback(RecognizerResult *result)
{
  printf("recognizer cb\n");

  return;
}

static void recognizer_find_callback(AsRecognitionInfo info)
{
 static const char *keywordstrings[] = {
    "Arm up",
    "Arm down",
    "Arm strech out",
    "Arm Bend",
    "pinchi in",
    "pinchi out",
    "lifr up",
    "take down",
    "turn left",
    "turn right",
    "",
    "",
    "",
    "",
    ""
  };

 static const char event_char[] = {
    'u',
    'd',
    's',
    'b',
    'i',
    'o',
    'f',
    't',
    'l',
    'r',
    '*',
    '*',
    '*',
    '*',
    '*'
  };

  printf("Notify size %d byte\n", info.size);

  int8_t *param = (int8_t *)info.mh.getVa();

  for (uint32_t i = 0; i < info.size / sizeof(int8_t); i++)
    {
      if (param[i] == 1)
        {
          g_event_char = event_char[i];
          printf("%s\n", keywordstrings[i]);
        }
    }  return;
}

static void attention_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention!");

  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING) {
    exit(1);
  }
}


void setup() {

  /* Display menu */
  Serial.begin(115200);
//  menu();

  /* Initialize memory pools and message libs */
  initMemoryPools();
  createStaticPools(MEM_LAYOUT_PLAYERANDRECOGNIZER);
//  createStaticPools(MEM_LAYOUT_RECOGNIZER);

  /* Initialize PWM */
  pwm.begin();

  // In theory the internal oscillator is 25MHz but it really isn't
  // that precise. You can 'calibrate' by tweaking this number till
  // you get the frequency you're expecting!
  pwm.setOscillatorFrequency(27000000);  // The int.osc. is closer to 27MHz
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates

  /* Use SD card */
  theSD.begin();

  /* Start audio system */

  theFrontend = FrontEnd::getInstance();
  theRecognizer = Recognizer::getInstance();

  theFrontend->begin();
  theRecognizer->begin(attention_cb);

  /* Set rendering clock */
  theFrontend->activate(frontend_done_callback);
  theRecognizer->activate(recognizer_done_callback);


  usleep(100 * 1000);

  theFrontend->init(recognizer_channel_number, recognizer_bit_length, 384,
                    ObjectConnector::ConnectToRecognizer, AsMicFrontendPreProcSrc, "/mnt/sd0/BIN/SRC");
//                    ObjectConnector::ConnectToRecognizer, AsMicFrontendPreProcSrc, "/mnt/spif/BIN/SRC");

  theRecognizer->init(recognizer_find_callback,
                      "/mnt/sd0/BIN/RCGPROC");
//                      "/mnt/spif/BIN/RCGPROC");

  /* Init Recognizer DSP */
  struct InitRcgProc : public CustomprocCommand::CmdBase
  {
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;
    uint32_t param4;
  };

  static InitRcgProc s_initrcgproc;
  s_initrcgproc.param1 = 500;
  s_initrcgproc.param2 = 600;
  s_initrcgproc.param3 = 0;
  s_initrcgproc.param4 = 0;

  theRecognizer->initRecognizerDsp((uint8_t *)&s_initrcgproc, sizeof(s_initrcgproc));

  /* Set Recognizer DSP */

  struct SetRcgProc : public CustomprocCommand::CmdBase
  {
    uint32_t param1;
    int32_t param2;
    int32_t param3;
    int32_t param4;
  };

  static SetRcgProc s_setrcgproc;
  s_setrcgproc.param1 = true;
  s_setrcgproc.param2 = -1;
  s_setrcgproc.param3 = -1;
  s_setrcgproc.param4 = -1;

  theRecognizer->setRecognizerDsp((uint8_t *)&s_setrcgproc, sizeof(s_setrcgproc));

  /* Start Recognizer */

  ledOn(LED0);

  theFrontend->start();
  theRecognizer->start();
  puts("Recognizing Start!");

  /* Initial Posture*/
  pwm.writeMicroseconds(CHUCK,INI_CHUCK_POS);
  pwm.writeMicroseconds(UPPER,INI_UPPER_POS);
  pwm.writeMicroseconds(LOWER,INI_LOWER_POS);
  pwm.writeMicroseconds(BASE ,INI_BASE_POS);
  delay(2);

}

void liftup()
{
  /* arm down */
  pos_joint(UPPER,80);
  pos_joint(LOWER,20);
  pos_joint(CHUCK,30);
  sleep(1);

  /* pinch in */
  pos_joint(CHUCK,90);
  sleep(1);

  /* arm up */
  pos_joint(UPPER,110);
  pos_joint(LOWER,90);
  sleep(2);

}

void takedown()
{
  /* arm down */
  pos_joint(UPPER,80);
  pos_joint(LOWER,22);
  sleep(1);

  /* pinch out */
  pos_joint(CHUCK,50);
  sleep(1);

  /* arm up */
  pos_joint(UPPER,110);
  pos_joint(LOWER,90);
  sleep(2);

}

void loop() {

  static uint8_t chuck_pos = INI_CHUCK_POS;
  static uint8_t upper_pos = INI_UPPER_POS;
  static uint8_t lower_pos = INI_LOWER_POS;
  static uint8_t base_pos  = INI_BASE_POS;

  /* Menu operation */

      switch (g_event_char) {
        case 'u': // arm up
          lower_pos += 15;
          pos_joint(LOWER,lower_pos);
          g_event_char = '*';
          break;
        case 'd': // arm doown
          lower_pos -= 15;
          pos_joint(LOWER,lower_pos);
          g_event_char = '*';
          break;
        case 'r': // turn right
          base_pos += 30;
          pos_joint(BASE,base_pos);
          g_event_char = '*';
          break;
        case 'l': // turn left
          base_pos -= 30;
          pos_joint(BASE,base_pos);
          g_event_char = '*';
          break;
        case 's': // stretch out
          upper_pos -= 15;
          pos_joint(UPPER,upper_pos);
          g_event_char = '*';
          break;
        case 'b': // bend
          upper_pos += 15;
          pos_joint(UPPER,upper_pos);
          g_event_char = '*';
          break;
        case 'i': // pinch in
          chuck_pos += 15;
          pos_joint(CHUCK,chuck_pos);
          g_event_char = '*';
          break;
        case 'o': // pinch out
          chuck_pos -= 15;
          pos_joint(CHUCK,chuck_pos);
          g_event_char = '*';
          break;
        case 'f': // lift up
          liftup();
          g_event_char = '*';
          break;
        case 't': // take down
          takedown();
          g_event_char = '*';
          break;
        default:
          break;
        }
//      printf("chuck = %d, upper = %d, lower = %d, base = %d\n",chuck_pos,upper_pos,lower_pos,base_pos);

      usleep(100*1000);
}
