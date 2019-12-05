//cec Talker Spresense向けサンプルコード
//HDMI TX 及び、HDMI RX Port0, Port1 にSource機器を接続した状態で動作します。
// written by 

#include <Audio.h>

AudioClass *theAudio;

/**
 * @brief Audio attention callback
 *
 * When audio internal error occurc, this function will be called back.
 */

static void audio_attention_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention!");
  
  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING)
    {
      exit(1);
   }
}


int init_complete = 0;

/*char* command_table[] = {
  "p 0",
  "p 1",
  "p 2",
  "p 3",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
}*/

void setup() {
  Serial.begin(115200);
  Serial2.begin(57600,SERIAL_8N1);
  Serial2.setTimeout(10000);

  // start audio system
  theAudio = AudioClass::getInstance();
  theAudio->begin(audio_attention_cb);

  puts("initialization Audio Library");

  /* Set output device to speaker with first argument.
   * If you want to change the output device to I2S,
   * specify "AS_SETPLAYER_OUTPUTDEVICE_I2SOUTPUT" as an argument.
   * Set speaker driver mode to LineOut with second argument.
   * If you want to change the speaker driver mode to other,
   * specify "AS_SP_DRV_MODE_1DRIVER" or "AS_SP_DRV_MODE_2DRIVER" or "AS_SP_DRV_MODE_4DRIVER"
   * as an argument.
   */
//  int err = theAudio->setThroughMode(AudioClass::BothIn,AudioClass::Mic,true,160,AS_SP_DRV_MODE_LINEOUT);
//  int err = theAudio->setThroughMode(AudioClass::I2sIn,AudioClass::Mixer,true,160,AS_SP_DRV_MODE_LINEOUT);
  int err = theAudio->setThroughMode(AudioClass::I2sIn,AudioClass::None,true,100,AS_SP_DRV_MODE_4DRIVER);
  if (err != AUDIOLIB_ECODE_OK)
    {
      printf("Through initialize error\n");
      exit(1);
    }

  puts("Set Audio Volume");
  theAudio->setVolume(-100);
  if (err != AUDIOLIB_ECODE_OK)
    {
      printf("Set Volume error\n");
      exit(1);
    }
}

void loop() {
  //初期化処理
  printf("Init!\n");
  init();
  printf("Init end!\n");

  while(true){
   if(init_complete == 0){
      //初期化未完了

  printf("Init Incomplete!\n");

     //初期化未完了時は、cecTalkerから"##"が送られてくることを待ち、
     //"##" が送られてきたら再度初期化処理を実行します
     while(true){
       String mess = getmessage();
       if( mess.equals("##") ){
         break;
       }
     }
     init();

   } else {
    //初期化処理完了
    //初期化処理が完了すると、init_completeに1が設定され、cecTalkerへの
    //指示が行えるようになります。

  printf("Init Complete!\n");

    //以下のコードは、psコマンドの送付を行い、HDMI RXのポート切り替えを
    //実行するサンプルになります。

    //port status チェック
     sendmessage("ps");

    // 1秒 wait
     delay(1000);

    //HDMI RX Port 0へ切り替え
     sendmessage("p 1");
    puts("Change 0");

    // 10秒 wait
     delay(10000);   

    //HDMI RX Port 0へ切り替え
     sendmessage("pl");
    puts("play");

    // 10秒 wait
     delay(10000);   

    sendmessage("bs");
    puts("prev");

    // 10秒 wait
     delay(10000);   

    sendmessage("ff");
    puts("FF");

    // 10秒 wait
     delay(10000);   

    sendmessage("ff");
    puts("FF");

    // 10秒 wait
     delay(10000);   

    sendmessage("pl");
    puts("Play");

    // 10秒 wait
     delay(10000);   

    //HDMI RX Port 1へ切り替え
//     sendmessage("p 1");
    puts("Change 1");

    // 10秒 wait
     delay(10000);

    //HDMI RX Port 1へ切り替え
//     sendmessage("p 2");
    puts("Change 2");

    // 10秒 wait
     delay(10000);

    //HDMI RX Port 0へ切り替え
     sendmessage("st");
    puts("stop");

    // 10秒 wait
     delay(10000);
    
    //HDMI RX Port 1へ切り替え
//     sendmessage("pw");
//    puts("Power");

    // 10秒 wait
//     delay(10000);

   }
  }

}

void init() {

  sendmessage("ps");

  printf("send ps!\n");

  String mess = getmessage();
  Serial.println(mess);

  /*while(true){
    String mess = getmessage();

    if( mess.equals("200") ){
      Serial.println("2s0");
      sendmessage("2s0");
      init_complete = 0;
    } else if (mess.charAt(0) == '3'){
      if(mess.charAt(2) == '0'){
        Serial.println("2s0");
        sendmessage("2s0");
        init_complete = 0;
        return;
      } else {
        Serial.println("2s1");
        sendmessage("2s1");
        init_complete = 1;
        return;
      }
    }
  }*/
        init_complete = 1;
}

String getmessage(){
  while (!Serial2.available());
  String mess = Serial2.readStringUntil('\r\n');
  mess.trim();
  return mess;
}

int sendmessage(String sndcmd){
  while(true){
    // Ctrl + M発行
    Serial2.println("");
    for(int i=0; i < 10; i++){
        printf("poo!\n");

      while (!Serial2.available());
      char mess = Serial2.read();
        printf("poo1!\n");
      
      //wait prompt
      if(mess == '>'){
        char buf[5];
        int len=sndcmd.length();
        sndcmd.toCharArray(buf,len+1); 

        for(int j=0; j<len; j++){
          Serial2.print(buf[j]);
          delay(50);
        }
        Serial2.println("");

        //wait command send complete
        while(true){
          while (!Serial2.available());
          char mess2 = Serial2.read();
          if(mess2 == '*'){
            return 0;
          }
        }
      }
    }
    
    delay(2000);
  }
}
