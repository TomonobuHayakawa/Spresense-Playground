/*
 *  SPRESENSE_WiFi.ino - GainSpan WiFi Module Control Program
 *  Copyright 2019 Norikazu Goto
 *
 *  This work is free software; you can redistribute it and/or modify it under the terms 
 *  of the GNU Lesser General Public License as published by the Free Software Foundation; 
 *  either version 2.1 of the License, or (at your option) any later version.
 *
 *  This work is distributed in the hope that it will be useful, but without any warranty; 
 *  without even the implied warranty of merchantability or fitness for a particular 
 *  purpose. See the GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License along with 
 *  this work; if not, write to the Free Software Foundation, 
 *  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <GS2200Hal.h>
#include <GS2200AtCmd.h>
#include "AppFunc.h"
#include "config.h"
#include <Audio.h>
#include <LowPower.h>
#include <Wire.h>
#include "KX122.h"

KX122 kx122(KX122_DEVICE_ADDRESS_1F);
AudioClass *theAudio;
RadioList theRadioList;

#define  CONSOLE_BAUDRATE  115200

char server_cid = 0;

uint8_t* ESCBuffer_p;
extern uint8_t ESCBuffer[];
extern uint32_t ESCBufferCnt;

enum State {
  E_RadioStart,
  E_AudioStart,
  E_Run,
  E_AudioStop,
  E_RadioStop,
  StateNum
};

State state = E_RadioStart;

/**
 * @brief Audio attention callback
 *
 * When audio internal error occurc, this function will be called back.
 */

static void audio_attention_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention!");
  
//  if (atprm->error_code > AS_ATTENTION_CODE_WARNING)
  if (atprm->error_att_sub_code == AS_ATTENTION_SUB_CODE_SIMPLE_FIFO_UNDERFLOW)
   {
      state = E_AudioStop;
     digitalWrite( LED2, HIGH ); // turn on LED
     digitalWrite( LED3, HIGH ); // turn on LED
   }else if(atprm->error_att_sub_code == AS_ATTENTION_SUB_CODE_DSP_EXEC_ERROR){
      // Don't Care.
   }else{
      sleep(1);
      LowPower.reboot();
   }
}

// the setup function runs once when you press reset or power the board
void setup()
{
  LowPower.begin();
  
	/* initialize digital pin LED_BUILTIN as an output. */
	pinMode(LED0, OUTPUT);
	digitalWrite( LED0, LOW );   // turn the LED off (LOW is the voltage level)
	Serial.begin( CONSOLE_BAUDRATE ); // talk to PC

	/* Initialize SPI access of GS2200 */
	Init_GS2200_SPI();

	digitalWrite( LED0, HIGH ); // turn on LED
  
  // start audio system
  theAudio = AudioClass::getInstance();
  theAudio->begin(audio_attention_cb);
  puts("initialization Audio Library");
  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, AS_SP_DRV_MODE_4DRIVER);
  err_t err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO);
  puts("initialization Player Library");

  theAudio->setVolume(-200);
  
	/* WiFi Module Initialize */
	App_InitModule();
  App_ConnectAP();
  
  digitalWrite( LED0, LOW );  // turn off LED
  digitalWrite( LED1, HIGH ); // turn on LED

  Wire.begin();
  
  // start Sensor
  int rc = kx122.init();
  if (rc != 0) {
    Serial.println(F("KX122 initialization failed"));
    Serial.flush();
  }
}

void start_radio()
{
  do{
    puts(theRadioList.ip());
    server_cid = App_ConnectWeb(theRadioList.ip(),theRadioList.port());
  }while(server_cid == ATCMD_INVALID_CID);
  
  digitalWrite( LED1, LOW );  // turn off LED
  digitalWrite( LED2, HIGH ); // turn on LED
}

void write_StartRadio(uint8_t* pt, uint32_t sz)
{
  while(sz > 3){
    if( ('\r' == *(pt)) &&
        ('\n' == *(pt+1)) &&
        ('\r' == *(pt+2)) &&
        ('\n' == *(pt+3)) ){
      state = E_AudioStart;
      pt = pt+3;
      sz = sz-3;
      ConsoleLog( "\nfind\n");
      break;
    }
    pt++;
    sz--;
  }

  if(state == E_AudioStart){
    write_StartAudio(pt, sz);
  }
}

const int start_size = 8000;
void write_StartAudio(uint8_t* pt, uint32_t sz) {
  static int buffered_size = 0;
  buffered_size += sz;
  if(buffered_size > start_size){
    puts("start");
    theAudio->startPlayer(AudioClass::Player0);
    state = E_Run;
    buffered_size = 0;
    digitalWrite( LED2, LOW );  // turn off LED
    digitalWrite( LED3, HIGH ); // turn on LED
  }
  write_Run(pt,sz);
}

void write_Run(uint8_t* pt, uint32_t sz) {
  err_t err = AUDIOLIB_ECODE_SIMPLEFIFO_ERROR;
  while(err == AUDIOLIB_ECODE_SIMPLEFIFO_ERROR){
    err = theAudio->writeFrames(AudioClass::Player0, pt, sz);
    usleep(10000);
  }
}

static int es_reader(int argc, FAR char *argv[])
{
  ATCMD_RESP_E resp;

  (void)argc;
  (void)argv;

while(1){

  /*Wait for data.*/
//  puts("loop");
  for( int i= 200;Get_GPIO37Status()==0;i--){
    if(i==0) {
      puts("Reboot!");
      LowPower.reboot();
    }
    usleep(10000);
  }

  while( Get_GPIO37Status() ){
    resp = AtCmd_RecvResponse();

    if( ATCMD_RESP_BULK_DATA_RX == resp ){
      if( Check_CID( server_cid ) ){
        
        ESCBuffer_p = ESCBuffer+1;
        ESCBufferCnt = ESCBufferCnt -1;

        switch(state){
          case E_RadioStart:
            write_StartRadio(ESCBuffer_p, ESCBufferCnt);
            break;
          case E_AudioStart:
            write_StartAudio(ESCBuffer_p, ESCBufferCnt);
            break;
          case E_Run:
            write_Run(ESCBuffer_p, ESCBufferCnt);
            break;
          case E_AudioStop:
            theAudio->stopPlayer(AudioClass::Player0,AS_STOPPLAYER_NORMAL);
            state = E_AudioStart;
            digitalWrite( LED3, LOW );  // turn off LED
            digitalWrite( LED2, HIGH ); // turn on LED
            break;
          case E_RadioStop:
            theAudio->stopPlayer(AudioClass::Player0,AS_STOPPLAYER_NORMAL);
            digitalWrite( LED3, LOW );  // turn off LED
            digitalWrite( LED2, LOW );  // turn off LED
            digitalWrite( LED1, HIGH ); // turn on LED
            state = E_RadioStart;
            WiFi_InitESCBuffer();
            sleep(10000);
            break;
          default:
            puts("error!");
            exit(1);
        }
      }
      WiFi_InitESCBuffer();
    }
  }
}

}

int radio_index = 0;

// the loop function runs over and over again forever
void loop() {

  state = E_RadioStart;
  start_radio();

//	ConsoleLog( "Start to send TCP Data");
//	Prepare for the next chunck of incoming data
	WiFi_InitESCBuffer();

//	Start the infinite loop to send the data
	if( ATCMD_RESP_OK != AtCmd_SendBulkData( server_cid, theRadioList.site(), strlen(theRadioList.site()) ) ){
		// Data is not sent, we need to re-send the data
		ConsoleLog( "Send Error.");
		delay(1);
	}

  puts(theRadioList.name());
  int tid = task_create("es_reader", 155, 1024, es_reader, NULL);

  while(1){
    byte rc;
    float acc[3];

    rc = kx122.get_val(acc);
    if (rc == 0) {
      if(acc[0] > 0.8){
        theRadioList.next();
        puts("Next!");
        state = E_RadioStop;
        sleep(1);
        task_delete(tid);
        break;
      }else if(acc[0] < -0.8){
        theRadioList.prev();
        puts("Prev!");      
        state = E_RadioStop;
        sleep(1);
        task_delete(tid);
        break;
      }
    }
    sleep(1);
  }
}
