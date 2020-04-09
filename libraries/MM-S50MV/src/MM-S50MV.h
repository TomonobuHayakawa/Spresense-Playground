/*
 *  MM-S50MV.h - MM-S50MV Library Header
 *  Copyright 2020 Tomonobu Hayakawa
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

class MMS50MVClass{
  
public:
  void begin();
  void end();

  void skip(int);
  void sync();
  void sync(uint8_t);

  void set(uint8_t cmd, uint8_t val);
  void led(uint8_t,uint8_t,uint8_t);

  int32_t get();
  bool get(int32_t*);

private:
  uint8_t buffer[256];

};

extern MMS50MVClass MMS50MV;
