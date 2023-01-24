/*
 *  MM-S50MV.cpp - MM-S50MV Library
 *  Author Tomonobu Hayakawa
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

#include "MM-S50MV.h"

// #define MMS50MV_DEBUG
#ifdef MMS50MV_DEBUG
#define mms50mv_printf printf
#else
#define mms50mv_printf(...) do {} while (0)
#endif

/*****************************************************************************/
/* Public                                                                    */
/*****************************************************************************/
void MMS50MVClass::begin()
{
  SPI5.begin();
  SPI5.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE3));
}

void MMS50MVClass::end()
{
  SPI5.end();
}

void MMS50MVClass::skip(int cnt)
{
  while (cnt > 0) {
    SPI5.transfer(0);
    cnt--;
  }
}

void MMS50MVClass::sync()
{
  int cnt = SPI5.transfer(0) & 0xff;

  while (cnt > 0) {
    SPI5.transfer(0);
    cnt--;
  }
}

void MMS50MVClass::sync(uint8_t id)
{
  int data = SPI5.transfer(0) & 0xff;

  while (id != data) {
    data = SPI5.transfer(0);
  }
}

void MMS50MVClass::set(uint8_t cmd, uint8_t val)
{
  memset(buffer,0,MMS50MV_DATA_SIZE);
  buffer[0] = 0xeb;
  buffer[1] = cmd;
  buffer[2] = 0x01;
  buffer[3] = val;
  buffer[4] = 0xed;

  SPI5.transfer(buffer,MMS50MV_DATA_SIZE);
}

void MMS50MVClass::led(uint8_t r, uint8_t g, uint8_t b)
{
  memset(buffer,0,MMS50MV_DATA_SIZE);
  
  buffer[12] = 0xeb;
  buffer[13] = 0xc2;
  buffer[14] = 0x01;
  buffer[15] = b;
  buffer[16] = 0xed;

  buffer[20]  = 0xeb;
  buffer[21]  = 0xc0;
  buffer[22]  = 0x01;
  buffer[23]  = r;
  buffer[24]  = 0xed;

  buffer[32]  = 0xeb;
  buffer[33]  = 0xc1;
  buffer[34]  = 0x01;
  buffer[35]  = g;
  buffer[36]  = 0xed;

  SPI5.transfer(buffer,MMS50MV_DATA_SIZE);
}

int32_t MMS50MVClass::get1d()
{
  int32_t result;
  get_data();
  result = ((buffer[4] << 24)|(buffer[5] << 16)|(buffer[6] << 8)|buffer[7]) / (0x400000 / 1000);
  return result;
}

uint16_t MMS50MVClass::get1p()
{
  get_data();
  uint16_t result = ((buffer[8] << 8)|buffer[9]) / 0x10;
  return result;
}

void MMS50MVClass::get3d(int32_t* ptr)
{
  get_data();
  for(int i=10;i<(4*8*4+10);i+=4,ptr++){
    *ptr = (((buffer[i] << 24) | (buffer[i+1] << 16) | (buffer[i+2] << 8)| buffer[i+3]) / (0x400000 / 1000));
  }
  
  return;
}

void MMS50MVClass::get3p(uint16_t* ptr)
{
  get_data();
  for ( int i = 4*8*4+10; i < (4*8*4+10+4*8*2); i += 2, ptr++ ) {
    *ptr = ((buffer[i] << 8)|buffer[i+1]) / 0x10;
  }
  return;
}

/*****************************************************************************/
/* Private                                                                   */
/*****************************************************************************/
bool MMS50MVClass::check_magic(void)
{
  bool ret = false;
  if (buffer[0] != 0xe9){
    mms50mv_printf("magic error! %x\n",buffer[0]);
  }
  else {
    ret = true;
  }
  return ret;
}

bool MMS50MVClass::check_sequence_id(void)
{
  bool ret = false;
  if (buffer[1] != buffer[255]){
    mms50mv_printf("sequence id error! %d,%d\n",buffer[1],buffer[255]);
  }
  else {
    ret = true;
  }
  return ret;
}

void MMS50MVClass::get_data(void)
{
  bool result = false;
  while (1) {
    SPI5.transfer(buffer,MMS50MV_DATA_SIZE);
    result = check_magic();
    if (!result) {
      continue;
    }
    else {
      result = check_sequence_id();
      if (!result) {
        continue;
      }
      else {
        break;
      }
    }
  }
  return;
}


/* Pre-instantiated Object for this class */
MMS50MVClass MMS50MV;
