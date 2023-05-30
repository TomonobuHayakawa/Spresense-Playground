/*
 *  RotaryEncoder.cpp - Rotary Encoder Library
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

#include "RotaryEncoder.h"

RotaryEncoder::RotaryEncoder(){

  cur1 = 0;
  cur2 = 0;
  state = 0;
  counter = 0;
}

bool RotaryEncoder::set(int new1, int new2)
{
  /* no updates */
  if(new1 == cur1 && new2 == cur2) return false;
  
  int new_state = state_table[new1][new2];
  cur1 = new1;
  cur2 = new2;

  int diff = counter_table[state][new_state];
  counter += diff;
  state = new_state;
  
  if(diff == 0) return false;
  return true;
}

int RotaryEncoder::get(){
  return counter;
}

void RotaryEncoder::reset(){
  counter = 0;
}
