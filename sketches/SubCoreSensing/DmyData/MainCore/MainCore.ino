/*
 *  MainCore.ino - Subcore Sensing Example on MainCore
 *  Copyright 2022 Sony Semiconductor Solutions Corporation
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

#ifdef SUBCORE
#error "Core selection is wrong!!"
#endif

#include <MP.h>

int subcore = 1; /* Communication with SubCore1 */

void setup()
{
  int ret = 0;

  Serial.begin(115200);
  while (!Serial);

  /* Launch SubCore1 */
  ret = MP.begin(subcore);
  if (ret < 0) {
    printf("MP.begin error = %d\n", ret);
  }

}

void loop()
{
  int      ret;
  uint32_t rcvdata;
  int8_t   sndid = 100; /* do not use now */
  int8_t   rcvid;
  /* Data from SubCore */

  /* Timeout 10000 msec */
  MP.RecvTimeout(10000);

  ret = MP.Recv(&rcvid, &rcvdata, subcore);
  if (ret < 0) {
    printf("MP.Recv error = %d\n", ret);
  }

  printf("Recv: id=%d data=0x%08lx \n", rcvid, rcvdata);

  delay(1000);
}
