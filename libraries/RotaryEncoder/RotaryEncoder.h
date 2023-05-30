/*
 *  RotaryEncoder.cpp - Rotary Encoder Library Header
 *  Copyright 2023 Spresense Users
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
 
 #ifndef _ROTARY_ENCODER_H_
#define _ROTARY_ENCODER_H_

class RotaryEncoder{

public:
  RotaryEncoder();

  void init(int base){ counter = base; }
  bool set(int,int);
  int get();

  void reset();

  private:
    int cur1, cur2;
    int counter;
    int state;

  const int state_table[2][2] = {
    {0,3},
    {1,2}
  };

  const int counter_table[4][4] = {
    {0,1,0,-1},
    {-1,0,1,0},
    {0,-1,0,1},
    {1,0,-1,0}
  };

};

#endif /* _ROTARY_ENCODER_H_ */
