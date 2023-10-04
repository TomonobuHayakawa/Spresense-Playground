/*
 *  Oscillator.ino - Oscillator & Sequencer Application on SpreSynth.
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

#include <SDHCI.h>
#include <Synthesizer.h>
#include <OutputMixer.h>
#include <MemoryUtil.h>

#include <MP.h>

const int controller_core = 1;

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Maximum number of channels */

#define CHANNEL_NUMBER        8

/* Oscillator parameter */

/* Organ */
/*
#define OSC_WAVE_MODE         AsSynthesizerSinWave
#define OSC_CH_NUM            CHANNEL_NUMBER
#define OSC_EFFECT_ATTACK     20
#define OSC_EFFECT_DECAY      1
#define OSC_EFFECT_SUSTAIN    100
#define OSC_EFFECT_RELEASE    10
*/

/* Piano */
#define OSC_WAVE_MODE         AsSynthesizerRectWave
#define OSC_CH_NUM            CHANNEL_NUMBER
#define OSC_EFFECT_ATTACK     20
#define OSC_EFFECT_DECAY      500
#define OSC_EFFECT_SUSTAIN    1
#define OSC_EFFECT_RELEASE    1

/* DSP file path */

//#define DSPBIN_PATH           "/mnt/spif/BIN/OSCPROC"
#define DSPBIN_PATH           "/mnt/sd0/BIN/OSCPROC"

/* Set volume[db] */

#define VOLUME_MASTER         -40
#define VOLUME_PLAY           -160
#define VOLUME_SYNTH_MAX      -4

int volume_synth = VOLUME_SYNTH_MAX;

/****************************************************************************/
SDClass theSD;

OutputMixer *theMixer;
Synthesizer *theSynthesizer;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/**
 * @brief Audio(player1) attention callback
 *
 * When audio internal error occurc, this function will be called back.
 */
/**
 * @brief Audio(mixer) attention callback
 *
 */
static void attention_mixer_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention(mixer)!");
}

/**
 * @brief Audio(Synth) attention callback
 *
 */
static void attention_synth_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention(synth)!");
}

/****************************************************************************/
/**
 * @brief Mixer done callback procedure
 *
 * @param [in] requester_dtq    MsgQueId type
 * @param [in] reply_of         MsgType type
 * @param [in,out] done_param   AsOutputMixDoneParam type pointer
 */

static void outputmixer0_done_callback(MsgQueId              /*requester_dtq*/,
                                       MsgType               /*reply_of*/,
                                       AsOutputMixDoneParam* /*done_param*/)
{
  return;
}

/**
 * @brief Synthesizer done callback function
 *
 * @param [in] event        AsSynthesizerEvent type indicator
 * @param [in] result       Result
 * @param [in] sub_result   Sub result
 *
 * @return true on success, false otherwise
 */

static void synthesizer_done_callback(AsSynthesizerEvent /*event*/,
                                      uint32_t           /*result*/,
                                      void*              /*param*/)
{
  return;
}

/****************************************************************************/

class Note2Fs {
  typedef struct {
    uint8_t pitch;  
    int     fs;
  } Key;

private:
  const Key table[12] =
  {
    {33,55},
    {34,58},
    {35,62},
    {36,65},
    {37,69},
    {38,73},
    {39,78},
    {40,82},
    {41,87},
    {42,93},
    {43,98},
    {44,104}
  };

public:
  int getFs(uint8_t pitch){
    return table[(pitch-33)%12].fs*pow(2,((pitch-33)/12));
  }
};

Note2Fs theNote2Fs;

static int32_t volume    = -24;
static int32_t tempo     = 50;

static int pitches[8] = {48,48,48,48,60,60,60,60};
//static int on_off[8]  = {0, 0, 0, 0, 0, 0, 0, 0};
static int on_off[8]  = {1, 0, 0, 0, 0, 0, 0, 0};

#define MAX_SEQUENCE_NUMBER 5
static int sequence_number = 0;
static boolean sequence_table[MAX_SEQUENCE_NUMBER][12][8] = {
{
  {1,0,0,0,1,0,0,0},
  {0,1,0,0,0,1,0,0},
  {0,0,1,0,0,0,1,0},
  {0,0,0,1,0,0,0,1},
  {1,0,0,0,1,0,0,0},
  {0,1,0,0,0,1,0,0},
  {0,0,1,0,0,0,1,0},
  {0,0,0,1,0,0,0,1},
  {1,0,0,0,1,0,0,0},
  {0,1,0,0,0,1,0,0},
  {0,0,1,0,0,0,1,0},
  {0,0,0,1,0,0,0,1}
},
{
  {1,1,0,0,1,0,0,0},
  {0,1,0,0,0,1,0,0},
  {1,0,1,0,1,0,1,0},
  {0,0,0,1,0,0,0,1},
  {1,1,0,0,1,0,0,0},
  {0,1,0,0,0,1,0,0},
  {1,0,1,0,1,0,1,0},
  {0,0,0,1,0,0,0,1},  
  {1,1,0,0,1,0,0,0},
  {0,1,0,0,0,1,0,0},
  {1,0,1,0,1,0,1,0},
  {0,0,0,1,0,0,0,1}  
},
{
  {1,1,0,0,1,0,0,0},
  {0,1,0,0,0,1,0,0},
  {1,0,1,0,1,0,1,0},
  {0,0,0,1,0,0,0,1},
  {0,1,0,0,0,1,0,0},
  {1,0,1,0,1,0,1,0},
  {0,1,0,0,0,1,0,0},
  {1,0,1,0,1,0,1,0},
  {0,0,0,1,0,0,0,1},
  {0,1,0,0,0,1,0,0},
  {0,0,0,1,0,0,0,1},
  {0,0,0,1,0,0,0,1}
},
{
  {1,0,1,0,1,0,1,0},
  {0,1,0,0,0,1,0,0},
  {0,0,0,1,0,0,0,1},
  {1,0,1,0,1,0,1,0},
  {0,1,0,0,0,1,0,0},
  {0,0,0,1,0,0,0,1},
  {1,0,1,0,1,0,1,0},
  {0,1,0,0,0,1,0,0},
  {0,0,0,1,0,0,0,1},
  {1,0,1,0,1,0,1,0},
  {0,1,0,0,0,1,0,0},
  {0,0,0,1,0,0,0,1}
},
{
  {1,0,0,0,1,0,1,1},
  {0,1,0,0,0,1,1,0},
  {1,0,1,0,1,0,1,0},
  {0,0,0,1,0,1,1,0},
  {1,0,0,0,1,0,1,1},
  {0,1,0,0,0,1,1,0},
  {0,0,1,0,1,0,1,0},
  {0,0,0,1,0,1,1,0},
  {1,0,0,0,1,0,1,1},
  {0,1,0,0,0,1,1,0},
  {0,0,1,0,1,0,1,0},
  {0,0,0,1,0,1,1,0}
}
};

int sequencer(int argc, FAR char *argv[])
{
  int er;
  static int j=0;
  while(1){
    for(int i=0;i<8;i++){
      if(sequence_table[sequence_number][j][i] && on_off[i]){
        er = theSynthesizer->set(i, theNote2Fs.getFs(pitches[i]));
      }

      if(on_off[i]==-1){
        printf("set off ch=%d,pictches=%d\n",i,pitches[i]);
        on_off[i]=0;
        er = theSynthesizer->set(i, 0);
      }
    }
    j=(j+1)%8;
    usleep(tempo*10*1000);
  }
}

// -----------------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  /* Mount SD card */
  theSD.begin();

  /* Multi-Core library */
  int ret = MP.begin(controller_core);
  if (ret < 0) {
    printf("MP.begin error = %d\n", ret);
  }
  MP.RecvTimeout(10000);

  /* Multi-Core library */
  ret = MP.begin(2);
  if (ret < 0) {
    printf("MP.begin error = %d\n", ret);
  }

  /* Initialize memory pools and message libs */
  initMemoryPools();
  createStaticPools(MEM_LAYOUT_PLAYER);

  /* Get static audio modules instance */
  theMixer  = OutputMixer::getInstance();
  theSynthesizer = Synthesizer::getInstance();

  /* Begin objects */
  theMixer->activateBaseband();
  theSynthesizer->begin();
  
  /* Create objects */
  theMixer->create(attention_mixer_cb);
  theSynthesizer->create(attention_synth_cb);

  /* Activate objects */
  theSynthesizer->activate(synthesizer_done_callback, NULL);
  theMixer->activate(OutputMixer0, HPOutputDevice, outputmixer0_done_callback);

  /* Initialize synthesizer. */
  theSynthesizer->init(OSC_WAVE_MODE,
                       OSC_CH_NUM,
                       DSPBIN_PATH,
                       OSC_EFFECT_ATTACK,
                       OSC_EFFECT_DECAY,
                       OSC_EFFECT_SUSTAIN,
                       OSC_EFFECT_RELEASE);

  /* Set master volume, Player0 volume, Player1 volume */

  theMixer->setVolume(VOLUME_MASTER, volume_synth, VOLUME_PLAY);

#define OSC_WAVE_MODE         AsSynthesizerRectWave
#define OSC_CH_NUM            CHANNEL_NUMBER
#define OSC_EFFECT_ATTACK     20
#define OSC_EFFECT_DECAY      500
#define OSC_EFFECT_SUSTAIN    1
#define OSC_EFFECT_RELEASE    1

  theSynthesizer->set(4,
                      0,
                      100,
                      300,
                      1,
                      1);
                      
  theSynthesizer->set(5,
                      0,
                      200,
                      500,
                      1,
                      1);
                      
  theSynthesizer->set(6,
                      0,
                      4,
                      50,
                      1,
                      1);
                      
  theSynthesizer->set(7,
                      0,
                      20,
                      500,
                      30,
                      1);

  /* Start synthesizer */
  theSynthesizer->start();

  int seq = task_create("Sequencer", 95, 0x400, sequencer, (FAR char* const*) 0);

}

void loop()
{
  int er;

  int8_t   rcvid;
  uint32_t data;
  
  int ret = MP.Recv(&rcvid, &data, controller_core);
  if(ret < 0){
    puts("time out!");
    return;
  }

  if((rcvid / 10 ) == 2){
    int channel = rcvid % 10;
    if(data){
      on_off[channel] = 1;
      printf("Channel%d On\n",channel);
    }else{
      on_off[channel] = -1;
      printf("Channel%d Off\n",channel);
    }
  }
  
  if((rcvid / 10 ) == 1){
    int channel = rcvid % 10;
    pitches[channel] = data;
    printf("Channel%d pich =%d\n",channel, pitches[channel]);
  }
  
  if((rcvid / 10 ) == 3){
    int sw = rcvid % 10;
    if(sw == 0) volume = data;
    if(sw == 1) tempo  = data;
//    if(sw == 2) sequence_number = data % MAX_SEQUENCE_NUMBER;
    printf("Channel%d pich =%d\n",sw, data);
  }

  if((rcvid / 10 ) == 4){
    int sw = rcvid % 10;
    if(sw == 2) sequence_number = (sequence_number+1) % MAX_SEQUENCE_NUMBER;
    printf("Channel%d up =%d\n", sw, sequence_number);
  }

/*  if((rcvid / 10 ) == 1){
    int channel = rcvid % 10;
    if(data){
      er = theSynthesizer->set(channel, theNote2Fs.getFs(pitches[channel]));
      printf("Channel%d On\n",channel);
    }else{
      er = theSynthesizer->set(channel, 0);
      printf("Channel%d Off\n",channel);
    }
  }
  
  if((rcvid / 10 ) == 2){
    int channel = rcvid % 10;
    pitches[channel] = data;
    er = theSynthesizer->set(channel, theNote2Fs.getFs(pitches[channel]));
    printf("Channel%d pich =%d\n",channel, pitches[channel]);
  }
*/

}
