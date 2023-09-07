/*
 *  Main.ino - Demo application .
 *  Copyright 2023 T.Hayakawa
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
#include <Camera.h>
#include <USBSerial.h>

USBSerial UsbSerial;

// Please change the serial setting for user environment
#define SERIAL_OBJECT   UsbSerial
#define SERIAL_BAUDRATE 921600

// Please select the display size
//int16_t width = CAM_IMGSIZE_QVGA_H,    height = CAM_IMGSIZE_QVGA_V;
int16_t width = CAM_IMGSIZE_VGA_H,     height = CAM_IMGSIZE_VGA_V;
//int16_t width = CAM_IMGSIZE_HD_H,      height = CAM_IMGSIZE_HD_V;
//int16_t width = CAM_IMGSIZE_QUADVGA_H, height = CAM_IMGSIZE_QUADVGA_V;
//int16_t width = CAM_IMGSIZE_FULLHD_H,  height = CAM_IMGSIZE_FULLHD_V;
//int16_t width = CAM_IMGSIZE_5M_H,      height = CAM_IMGSIZE_5M_V;
//int16_t width = CAM_IMGSIZE_3M_H,      height = CAM_IMGSIZE_3M_V;

void CamCB(CamImage img)
{
  static int toggle = 0;
  static uint64_t base_time = 0;

  // for debug
  uint64_t now = millis();
  printf("time= %lld [ms]\n", now - base_time);
  base_time = now;
  (toggle++ & 1) ? ledOn(LED1) : ledOff(LED1);

  if (img.isAvailable()) {
    send_jpeg(img.getImgBuff(), img.getImgSize());
    uint64_t tdiff = millis() - now;
    printf("time= %lld [ms] %.3lf [Mbps]\n", tdiff, (float)(img.getImgSize() * 8) / tdiff / 1000);
  }
}

void setup()
{
  Serial.begin(115200);

  SERIAL_OBJECT.begin(SERIAL_BAUDRATE);
  Serial.println("Done!");

  Serial.println("Prepare camera");
  CamErr err = theCamera.begin(2,
                        CAM_VIDEO_FPS_15,
                        width,
                        height,
                        CAM_IMAGE_PIX_FMT_JPG);
  assert(err == CAM_ERR_SUCCESS);

  /* Auto white balance configuration */
  //err = theCamera.setAutoWhiteBalanceMode(CAM_WHITE_BALANCE_AUTO);
  //err = theCamera.setAutoWhiteBalanceMode(CAM_WHITE_BALANCE_INCANDESCENT);
  //err = theCamera.setAutoWhiteBalanceMode(CAM_WHITE_BALANCE_FLUORESCENT);
  err = theCamera.setAutoWhiteBalanceMode(CAM_WHITE_BALANCE_DAYLIGHT);
  //err = theCamera.setAutoWhiteBalanceMode(CAM_WHITE_BALANCE_FLASH);
  //err = theCamera.setAutoWhiteBalanceMode(CAM_WHITE_BALANCE_CLOUDY);
  //err = theCamera.setAutoWhiteBalanceMode(CAM_WHITE_BALANCE_SHADE);
  assert(err == CAM_ERR_SUCCESS);

  /* Start video stream */
  err = theCamera.startStreaming(true, CamCB);
  assert(err == CAM_ERR_SUCCESS);
}

void loop()
{
}

int send_jpeg(const uint8_t* buffer, size_t size)
{
//  usleep(200*1000);
  SERIAL_OBJECT.write('S'); // Payload
  SERIAL_OBJECT.write('P'); // Payload
  SERIAL_OBJECT.write('R'); // Payload
  SERIAL_OBJECT.write('S'); // Payload

  // Send a binary data size in 4byte
  SERIAL_OBJECT.write((size >> 24) & 0xFF);
  SERIAL_OBJECT.write((size >> 16) & 0xFF);
  SERIAL_OBJECT.write((size >>  8) & 0xFF);
  SERIAL_OBJECT.write((size >>  0) & 0xFF);

  // Send binary data
  size_t sent = 0;
  do {
    size_t s = SERIAL_OBJECT.write(&buffer[sent], size - sent);
    if ((int)s < 0) {
      return -1;
    }
    sent += s;
  } while (sent < size);

  return 0;
}
