
#include <MP.h>

#include "FFT.h"
#include "PitchScaleAdjuster.h"

/* Use CMSIS library */
#define ARM_MATH_CM4
#define __FPU_PRESENT 1U
#include <cmsis/arm_math.h>

/* Select FFT Offset */
const int g_channel = 1;

/* Allocate the larger heap size than default */

USER_HEAP_SIZE(64 * 1024);

/* MultiCore definitions */

struct Capture {
  void *buff;
  int  sample;
  int  chnum;
};

/*struct Result {
  float peak[g_channel];
  int  chnum;
};*/

struct Result {
  uint16_t peak[g_channel];
  uint16_t power[g_channel];
  int  chnum;
};


void setup()
{
  int ret = 0;

  /* Initialize MP library */
  ret = MP.begin();
  if (ret < 0) {
    errorLoop(2);
  }
  /* receive with non-blocking */
  MP.RecvTimeout(MP_RECV_POLLING);

  FFT.begin();
  PitchScaleAdjuster.begin();

  PitchScaleAdjuster.set(E_Major);
}

/* Tentative!!*/
float smoothing_peak(float cur)
{
  static float data[3] = {0,0,0};

  data[0] = cur;
  float ave = (data[2] + data[1] + data[0])/3;
  data[2] = data[1];
  data[1] = data[0];
//	printf("cur=%8.3f, %8.3f\n",cur,ave);
  return ave;
}

/* Tentative!!*/
float smoothing_pow(float cur)
{
  static float data[3] = {0,0,0};

  data[0] = cur;
  float ave = (data[2] + data[1] + data[0])/3;
  data[2] = data[1];
  data[1] = data[0];
//	printf("pow=%8.3f, %8.3f\n",cur,ave);
  return ave;
}

void loop()
{
  int      ret;
  int8_t   sndid = 10; /* user-defined msgid */
  int8_t   rcvid;
  Capture *capture;
  static Result   result;

  static float pDst[FFTLEN/2];

  /* Receive PCM captured buffer from MainCore */
  ret = MP.Recv(&rcvid, &capture);
  if (ret >= 0) {
//      printf("rev1 %d\n",capture->sample);
      FFT.put((q15_t*)capture->buff,capture->sample);
  }

  float peak[4];
  float power[4];
  while(FFT.empty(0) != 1){
    result.chnum = g_channel;
    for (int i = 0; i < g_channel; i++) {
      int cnt = FFT.get(pDst,i);
      peak[i] = get_peak_frequency(pDst, FFTLEN, &power[i]);
      result.peak[i] = (uint16_t)(PitchScaleAdjuster.get(smoothing_peak(peak[i])));
      result.power[i] = (uint16_t)(smoothing_pow(power[i])*100);
//      printf("Sub %8.3f, %8.3f\n", power[i],peak[i]);
//      printf("Sub %d, %d\n", result.power[i],result.peak[i]);

    }
    ret = MP.Send(sndid, &result,0);
    if (ret < 0) {
      errorLoop(1);
    }
  }

}

float get_peak_frequency(float *pData, int fftLen, float *power)
{
//  float g_fs = 48000.0f;
  float g_fs = 16000.0f;
  uint32_t index;
  float maxValue;
  float delta;
  float peakFs;

  arm_max_f32(pData, fftLen / 2, &maxValue, &index);
  *power = maxValue;

  delta = 0.5 * (pData[index - 1] - pData[index + 1])
    / (pData[index - 1] + pData[index + 1] - (2.0f * pData[index]));
  peakFs = (index + delta) * g_fs / (fftLen - 1);

  return peakFs;
}

void errorLoop(int num)
{
  int i;

  while (1) {
    for (i = 0; i < num; i++) {
      ledOn(LED0);
      delay(300);
      ledOff(LED0);
      delay(300);
    }
    delay(1000);
  }
}
