/*
 *  CurrentMonitor.ino - Example for battery current monitoring
 *  Copyright 2018 Sony Semiconductor Solutions Corporation
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

#include <LowPower.h>
#include <arch/board/board.h>
#include <arch/chip/pm.h>

unsigned int cycle = 10 * 1000 * 1000; // 10sec

volatile int mode = 0;
volatile int occurInt = 0;

unsigned int isr(void)
{
  occurInt = 1;
  return 0;
}

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  // Initialize LowPower library
  LowPower.begin();

  // Set cyclic timer
  attachTimerInterrupt(isr, cycle);
}

extern "C" {
uint32_t cxd56_get_cpu_baseclk(void);
}

#define NUM 200
volatile int g_sum = 0;
volatile int g_num = 0;

volatile float g_min = 0;
volatile float g_max = 0;
int currentSum;

void loop()
{
  float current;
  float avg;
  int i;

  if (occurInt) {
    printf("cpu=%d\n", cxd56_get_cpu_baseclk());
    if ((mode % 3) == 0) {
      LowPower.clockMode(CLOCK_MODE_HIGH);
      printf("cpu=%d\n", cxd56_get_cpu_baseclk());
      ledOn(LED0);
      ledOff(LED1);
      ledOff(LED2);
      printf("mode=%d\n", 0);
      detachTimerInterrupt();
      attachTimerInterrupt(isr, cycle);
    } else if ((mode % 3) == 1) {
      LowPower.clockMode(CLOCK_MODE_MIDDLE);
      printf("cpu=%d\n", cxd56_get_cpu_baseclk());
      ledOff(LED0);
      ledOn(LED1);
      ledOff(LED2);
      printf("mode=%d\n", 1);
      detachTimerInterrupt();
      attachTimerInterrupt(isr, cycle);
    } else if ((mode % 3) == 2) {
      LowPower.clockMode(CLOCK_MODE_LOW);
      printf("cpu=%d\n", cxd56_get_cpu_baseclk());
      ledOff(LED0);
      ledOff(LED1);
      ledOn(LED2);
      printf("mode=%d\n", 2);
      detachTimerInterrupt();
      attachTimerInterrupt(isr, cycle);
    }
    mode++;
    occurInt = 0;
  }

  currentSum = 0;
  for (i = 0; i < NUM; i++) {
    // Get the current
    currentSum += LowPower.getCurrent();
  }

  current = (float)currentSum / NUM;

  //printf("%.3lf, %.3lf, %.3lf\n", g_min, g_max, avg);
  //printf("%3.9lf\n", avg);
  printf("%3.9lf\n", current);

  //printf("cpu=%d\n", cxd56_get_cpu_baseclk());
  usleep(1 * 1000); // 1msec
}

