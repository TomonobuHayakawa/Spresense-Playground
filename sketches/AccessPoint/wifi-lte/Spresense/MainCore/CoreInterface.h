/*
 *  CoreInterface.h - Multi-Core Communication Header.
 *  Copyright 2024 T,Hayakawa
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

/* MultiCore definitions */
struct Request {
  uint8_t device;
  uint8_t enable;
  Request():device(0),enable(0){}
};

struct Result {
  uint8_t  device;
  uint16_t size;
  char     buffer[256];
  Result():device(0),size(0){}
};

#define NOMAL_REQUEST_ID 20
#define ERROR_REQUEST_ID 99

#define NOMAL_RESULT_ID 10
#define ERROR_RESULT_ID 99

#endif /* __COREINTERFACE_H__ */
