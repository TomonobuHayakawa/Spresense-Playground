/*
 *  SmartCajon.ino - Smart Cajon application
 *  Copyright 2019 Sony Semiconductor Solutions Corporation
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

#include <SDHCI.h>
#include <OutputMixer.h>
#include <MemoryUtil.h>
#include <stdio.h>
#include <stdlib.h>

#include <arch/board/board.h>
#include <arch/chip/cxd56_audio.h>  /* For set_datapath */
#define READSAMPLE (240)
#define BYTEWIDTH (2)
#define CHNUM (2)
#define READSIZE (READSAMPLE * BYTEWIDTH * CHNUM)

SDClass theSD;
OutputMixer *theMixer;

File myFile;

//sensor input
int gauge_a2=0;
int gauge_a3=0;
int gauge_a2_p1=0;
int gauge_a3_p1=0;
int gauge_a2_p2=0;
int gauge_a3_p2=0;
int gauge_a2_p3=0;

int detect_peak_a2;
int detect_peak_a3;

//toggle switch
#define OFF 0
#define ON 1
int toggle0 = OFF;
int toggle1 = OFF;
int cnttgl0 = 0;
int cnttgl1 = 0;



class BridgeBuffer {
private:

  const uint32_t mc_readsize = READSIZE;
  
  uint8_t m_buf[25 * 1024];
  uint32_t m_wp;
  uint32_t m_rp;
  
public:

  BridgeBuffer()
    : m_wp(0)
    , m_rp(0)
    { printf("bufsize %d\n", sizeof(m_buf)); }
  ~BridgeBuffer() {}

  uint32_t writebuf(File file)
  {
    return writebuf(file, mc_readsize);    
  }



  /* Read from source and write to buffer. */
  int32_t writebuf(File file, uint32_t reqsize)
  {
    /* Get vacant space. */
    uint32_t space = (m_wp < m_rp) ? (m_rp - m_wp) : (sizeof(m_buf) - m_wp + m_rp);
    uint32_t writesize = 0;

    /* If vacant space is smaller than request size, don't write buffer. */
    if (space < reqsize) {
      return -1;
    }

    /* Read file and write to buffer. */
    if (m_wp + reqsize > sizeof(m_buf)) {
      writesize = file.read(&m_buf[m_wp], sizeof(m_buf) - m_wp);
      writesize += file.read(&m_buf[0], reqsize - (sizeof(m_buf) - m_wp));
      m_wp = (m_wp + writesize) % sizeof(m_buf);
    }
    else {
      writesize = file.read(&m_buf[m_wp], reqsize);
      m_wp += writesize;
    }

    /* Return wrote size. */
    return writesize;
  }

  /* Read from buffer and move to destination. */
  uint32_t readbuf(uint8_t *dst)
  {
    /* Get remain size of data in the buffer. */
    uint32_t rem = (m_rp <= m_wp) ? (m_wp - m_rp) : (sizeof(m_buf) - m_rp + m_wp);
    uint32_t readsize = (rem > mc_readsize) ? mc_readsize : rem;
    //printf("wp %d rp %d rem %d read %d\n", m_wp, m_rp, rem, readsize);

    /* No data remains, then return. */
    if (readsize <= 0) {
      return readsize;
    }

    /* Read buffer and move to destination. */
    if (m_rp + readsize > sizeof(m_buf)) {
      memcpy(dst, &m_buf[m_rp], sizeof(m_buf) - m_rp);
      memcpy(dst + (sizeof(m_buf) - m_rp), &m_buf[0], readsize - (sizeof(m_buf) - m_rp));
      m_rp = (m_rp + readsize) % sizeof(m_buf);
    }
    else {
      memcpy(dst, &m_buf[m_rp], readsize);
      m_rp += readsize;
    }

    /* Return read size. */
    return readsize;
  }

  /* Clear buffer. */
  void clearbuf(void)
  {
    m_rp = 0;
    m_wp = 0;
  }
};

BridgeBuffer myBuffer;

bool ErrEnd = false;

static enum State {
  Ready = 0,
  Active,
  Stopping,
} s_state = Ready;

static bool getFrame(AsPcmDataParam *pcm)
{
  return getFrame(pcm, false);
}

static bool getFrame(AsPcmDataParam *pcm, bool direct_read)
{
//  uint16_t tmp[readsize * 2];

  /* Alloc MemHandle */
  if (pcm->mh.allocSeg(S0_REND_PCM_BUF_POOL, READSIZE) != ERR_OK) {
    return false;
  }

  /* Set PCM parameters */
  pcm->identifier = 0;
  pcm->callback = 0;
  pcm->bit_length = 16;
  pcm->size = (direct_read) ? myFile.read((uint8_t *)pcm->mh.getPa(), READSIZE) : myBuffer.readbuf((uint8_t *)pcm->mh.getPa());
  pcm->sample = pcm->size / BYTEWIDTH / CHNUM;  
  pcm->is_end = (pcm->size < READSIZE);
  pcm->is_valid = (pcm->size > 0);

  return true;
}

static bool start(uint8_t no)
{
  printf("start(%d) start\n", no);

  /* Open file placed on SD card */
  /* 16bit RAW data  */
  const char *raw_files[] =
  {
    "drum2_cymbal.raw",     //0
    "drum2_snare.raw",      //1
    "drum2_ride.raw",       //2
//    "drum2_kick.raw",       //3
    "cat15.raw",            //3
    "clap_01.raw",          //4
    "jzt_crsh_splashy.raw", //5
    "EK_hihat_openup.raw",  //6
    "christmasbell.raw",    //7
  };

  char fullpath[64] = { 0 };

  assert(no < sizeof(raw_files) / sizeof(char *));

  snprintf(fullpath, sizeof(fullpath), "AUDIO/%s", raw_files[no]);

  myFile = theSD.open(fullpath);

  if (!myFile)
    {
      printf("File open error\n");
      return false;
    }

  /* Start rendering. */
  for (int i = 0; i < 3; i++) {
    AsPcmDataParam pcm_param;
    if (!getFrame(&pcm_param, true)) {
      break;
    }

    /* Send PCM */
    int err = theMixer->sendData(OutputMixer0,
                                 outmixer_send_callback,
                                 pcm_param);

    if (err != OUTPUTMIXER_ECODE_OK) {
      printf("OutputMixer send error: %d\n", err);
      return false;
    }
  }

  /* Seek set to top of file, and clear buffer. */
  myBuffer.clearbuf();

  /* Buffer pre store. */
  myBuffer.writebuf(myFile, READSIZE * 3);
  
  printf("start() complete\n");

  return true;
}

static bool restart()
{
  printf("restart()\n");

  /* Seek set to top of file. */
  myFile.seek(0);

  for (int i = 0; i < 2; i++) {
    AsPcmDataParam pcm_param;
    if (getFrame(&pcm_param, true)) {
      /* Send PCM */
      int err = theMixer->sendData(OutputMixer0,
                                   outmixer_send_callback,
                                   pcm_param);

      if (err != OUTPUTMIXER_ECODE_OK) {
        printf("OutputMixer send error: %d\n", err);
        return false;
      }
    }
  }

  /* Clear buffer. */
  myBuffer.clearbuf();

  /* Store data. (Don't write a lot of frame at once.) */
  for (int i = 0; i < 3; i++) {
    if (myBuffer.writebuf(myFile) < 0) {
      break; 
    }
  }

  return true;
}

#if 0
static void stop()
{
  printf("stop\n");

  AsPcmDataParam pcm_param;
  getFrame(&pcm_param);

  pcm_param.is_end = true;

  int err = theMixer->sendData(OutputMixer0,
                               outmixer_send_callback,
                               pcm_param);

  if (err != OUTPUTMIXER_ECODE_OK) {
    printf("OutputMixer send error: %d\n", err);
  }

  /* Clear buffer. */
  myBuffer.clearbuf();
  myFile.close();
}
#endif
/**
 * @brief Mixer done callback procedure
 *
 * @param [in] requester_dtq    MsgQueId type
 * @param [in] reply_of         MsgType type
 * @param [in,out] done_param   AsOutputMixDoneParam type pointer
 */
static void outputmixer_done_callback(MsgQueId requester_dtq,
                                      MsgType reply_of,
                                      AsOutputMixDoneParam *done_param)
{
  //printf(">> %x done\n", reply_of);
  return;
}

/**
 * @brief Mixer data send callback procedure
 *
 * @param [in] identifier   Device identifier
 * @param [in] is_end       For normal request give false, for stop request give true
 */
static void outmixer_send_callback(int32_t identifier, bool is_end)
{
  //printf("send done %d %d\n", identifier, is_end);

  AsPcmDataParam pcm_param;

  while (true) {
    if (s_state == Stopping) {
      break;
    }

    if (!getFrame(&pcm_param)) {
      break;
    }
    
    /* Send PCM */
    //printf("send\n");
    pcm_param.is_end = false;
    pcm_param.is_valid = true;
    if (pcm_param.size == 0) {
      //printf("0 size\n");
      pcm_param.size = READSIZE;
      pcm_param.sample = pcm_param.size / BYTEWIDTH / CHNUM;
      memset(pcm_param.mh.getPa(), 0, pcm_param.size);
    }
    int err = theMixer->sendData(OutputMixer0,
                                 outmixer_send_callback,
                                 pcm_param);

    if (err != OUTPUTMIXER_ECODE_OK) {
      printf("OutputMixer send error: %d\n", err);
      break;
    }

/*
    if (pcm_param.is_end) {
      printf("FifoEnd\n");
      s_state = Stopping;
      myFile.close();
      myBuffer.clearbuf();
      break;
    }
*/
  }

  if (is_end) {
    s_state = Ready;
  }

  return;
}


static void attention_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention!");

  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING) {
    ErrEnd = true;
  }
}

void setup()
{
  printf("setup() start\n");

  /* pinsetup */

  //tact switch for PDA01
  pinMode( PIN_D12, INPUT_PULLUP );
  pinMode( PIN_D07, INPUT_PULLUP );
  pinMode( PIN_D06, INPUT_PULLUP );
  pinMode( PIN_D05, INPUT_PULLUP );
  pinMode( PIN_D09, INPUT_PULLUP );
  pinMode( PIN_D03, INPUT_PULLUP );
  pinMode( PIN_D11, INPUT_PULLUP );

  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);


  
  /* Display menu */

  Serial.begin(115200);
  
  /* Initialize memory pools and message libs */  
  initMemoryPools();
  createStaticPools(MEM_LAYOUT_PLAYER);

  /* Use SD card */
  theSD.begin();

  /* Start audio system */
  theMixer  = OutputMixer::getInstance();
  theMixer->activateBaseband();

  /* Create Objects */
  theMixer->create(attention_cb);

  /* Set rendering clock */
  theMixer->setRenderingClkMode(OUTPUTMIXER_RNDCLK_NORMAL);

  /* Activate Mixer Object.
   * Set output device to speaker with 2nd argument.
   * If you want to change the output device to I2S,
   * specify "I2SOutputDevice" as an argument.
   */
  theMixer->activate(OutputMixer0, HPOutputDevice, outputmixer_done_callback);

  usleep(100 * 1000);

  /* Input */
  cxd56_audio_en_input();
  cxd56_audio_mic_gain_t  mic_gain;

 // mic_gain.gain[0] = 210;
  mic_gain.gain[0] = 50;
  mic_gain.gain[1] = 0;
  mic_gain.gain[2] = 0;
  mic_gain.gain[3] = 0;
  mic_gain.gain[4] = 0;
  mic_gain.gain[5] = 0;
  mic_gain.gain[6] = 0;
  mic_gain.gain[7] = 0;

  cxd56_audio_set_micgain(&mic_gain);

  cxd56_audio_signal_t sig_id;
  cxd56_audio_sel_t    sel_info;

  sig_id = CXD56_AUDIO_SIG_MIC1;

  sel_info.au_dat_sel1 = false;
  sel_info.au_dat_sel2 = true;
  sel_info.cod_insel2  = false;
  sel_info.cod_insel3  = true;
  sel_info.src1in_sel  = false;
  sel_info.src2in_sel  = false;

  cxd56_audio_set_datapath(sig_id, sel_info);
  
  /* Set main volume */
  theMixer->setVolume(0, 0, 0); // master pcm_source mic

  /* Unmute */
  board_external_amp_mute_control(false);

  printf("setup() complete\n");
  digitalWrite(LED0, HIGH);
}


uint8_t start_event(uint8_t playno, uint8_t eventno)
{
  if (s_state == Ready) {
    if (start(eventno)) {
      s_state = Active;      
    }
  } else {
    if(playno == eventno){
      printf("Restart\n");
      restart();
    }else{
      myFile.close();
      start(eventno);
    }
  }
  return eventno;
}

void loop()
{

  err_t err = OUTPUTMIXER_ECODE_OK;
  uint8_t playno = 0xff;

  /* Fatal error */
  if (ErrEnd) {
    printf("Error End\n");
    digitalWrite(LED3, HIGH);
    goto stop_player;
  }

  /* Menu operation */



  //トグルスイッチ処理 DSW0 LED1
  //ボタンを押されたらトグルを有効にする
  if (digitalRead(PIN_D12) == LOW && toggle0 == OFF) {
    toggle0 = ON;
    cnttgl0++;
    digitalWrite(LED1, HIGH);
    //ボタンが押され続けている場合の処理
    while (digitalRead(PIN_D12) == LOW) {
    toggle0 = ON;
    digitalWrite(LED1, HIGH);
    }
  }
    //トグル動作中にボタンONでフラグを消す
  if (digitalRead(PIN_D12) == LOW && toggle0 == ON) {
    toggle0 = OFF;
    cnttgl0++;
    digitalWrite(LED1, LOW);
    //ボタンが押され続けている場合の処理
    while (digitalRead(PIN_D12) == LOW)
    {
    toggle0 = OFF;
    digitalWrite(LED1, LOW);
    }
  }


    //トグルスイッチ処理 DSW1  LED2
  //ボタンを押されたらトグルを有効にする
  if (digitalRead(PIN_D07) == LOW && toggle1 == OFF) {
    toggle1 = ON;
    cnttgl1++;
    digitalWrite(LED2, HIGH);
    //ボタンが押され続けている場合の処理
    while (digitalRead(PIN_D07) == LOW) {
    toggle1 = ON;
    digitalWrite(LED2, HIGH);
    }
  }
    //トグル動作中にボタンONでフラグを消す
  if (digitalRead(PIN_D07) == LOW && toggle1 == ON) {
    toggle1 = OFF;
    cnttgl1++;
    digitalWrite(LED2, LOW);
    //ボタンが押され続けている場合の処理
    while (digitalRead(PIN_D07) == LOW)
    {
    toggle1 = OFF;
    digitalWrite(LED2, LOW);
    }
  }




  //タッチセンサ処理
  //read analog input
  //preserve previous data for rising edge detection
  gauge_a2_p3 = gauge_a2_p2;
  gauge_a2_p2 = gauge_a2_p1;
  gauge_a2_p1 = gauge_a2;
  gauge_a2 = analogRead(A2);
  
  gauge_a3_p2 = gauge_a3_p1;
  gauge_a3_p1 = gauge_a3;
  gauge_a3 = analogRead(A3);  

 
  //detect rising edge with previous sample

   detect_peak_a2 =( gauge_a2 >gauge_a2_p1 )?1:0;
   detect_peak_a3 =( gauge_a3 >gauge_a3_p1 )?1:0;



  //threshold input and start event

 //PAD 00 (A2 pin)
  if(detect_peak_a2==1 && gauge_a2 >500){  
    printf("gauge_a2= %d gauge_a2_p1= %d gauge_a2_p2= %d toggle0= %d  cnt0 = %d toggle1= %d cnt1 = %d \n" ,gauge_a2,gauge_a2_p1,gauge_a2_p2,toggle0,cnttgl0,toggle1,cnttgl1);  
    playno = start_event(playno,cnttgl0%8);

 //PAD 01 (A3 pin)   
  }else if(detect_peak_a3==1 && gauge_a3 >500){  
    printf("gauge_a3= %d gauge_a3_p1= %d gauge_a3_p2= %d toggle0= %d cnt0=%d toggle1= %d cnt1 =%d\n" ,gauge_a3,gauge_a3_p1,gauge_a3_p2,toggle0,cnttgl0,toggle1,cnttgl1);  
    playno = start_event(playno,cnttgl1%8);
  }

  
  /* Processing in accordance with the state */

  switch (s_state) {
    case Stopping:
      break;
    case Ready:
      break;
    case Active:
      /* Send new frames to be decoded until end of file */
      //printf("active\n");
      for(int i = 0; i < 10; i++) {
        /* get PCM */
        int32_t wsize = myBuffer.writebuf(myFile);
        if (wsize < 0) {
          break;
        }
        else if (wsize < READSIZE) {
          break;        
        }
        else {
        }
      }
      break;

    default:
      break;
  }

  /* This sleep is adjusted by the time to read the audio stream file.
     Please adjust in according with the processing contents
     being processed at the same time by Application.
   */

  usleep(1 * 5000);
//  usleep(1 * 100000);
  return;

stop_player:
  printf("Exit player\n");
  myFile.close();
  exit(1);
}
