/*
 *  CoreInterface.h - Core interface header on the frequency characteristic viewer.
 *  Copyright 2023 T,Hayakawa
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
#ifndef __COREINTERFACE_H__
#define __COREINTERFACE_H__

#include <MP.h>

#if (SUBCORE != 1)
#error "Core selection is wrong!!"
#endif

/* MultiCore definitions */
struct Request {
  uint16_t *buffer;
  uint16_t  sample;
  uint32_t frame_no;
  uint16_t  channel;
  Request():buffer(0),sample(0){}
};

struct Result {
  uint16_t *buffer;
  uint16_t  sample;
  uint32_t frame_no;
  uint16_t  channel;
  Result():buffer(0),sample(0){}
};

#define NOMAL_REQUEST_ID 20
#define ERROR_REQUEST_ID 99

#define NOMAL_RESULT_ID 10
#define ERROR_RESULT_ID 99

#endif /* __COREINTERFACE_H__ */
