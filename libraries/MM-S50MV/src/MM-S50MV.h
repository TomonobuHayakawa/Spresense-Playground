/*
 *  MM-S50MV.h - MM-S50MV Library Header
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

#include <SPI.h>

/*------------------------------------------------------------------*/
/* Definitions                                                      */
/*------------------------------------------------------------------*/

#define MMS50MV_CMD_MODE     0x00
#define MMS50MV_MODE_NOMAL   0x00
#define MMS50MV_MODE_SYNC    0xFF

#define MMS50MV_RATE_LOW     0x10
#define MMS50MV_RATE_HIGH    0x11

#define MMS50MV_CMD_DISTANCE 0x12
#define MMS50MV_DIST_SHORT   0x00
#define MMS50MV_DIST_LONG    0x01

#define MMS50MV_CMD_STANDBY  0x80
#define MMS50MV_ACTIVE       0x00
#define MMS50MV_STANDBY      0x01


/*------------------------------------------------------------------*/
/* Class                                                            */
/*------------------------------------------------------------------*/
class MMS50MVClass{
  
public:
  void begin();
  void end();

  void skip(int);
  void sync();
  void sync(uint8_t);

  void set(uint8_t cmd, uint8_t val);
  void led(uint8_t,uint8_t,uint8_t);

  int32_t  get(){ return get1d(); }
  int32_t  get1d();
  uint16_t get1p();
  bool     get3d(int32_t*);
  bool     get3p(uint16_t*);

private:
  uint8_t buffer[256];

};

extern MMS50MVClass MMS50MV;
