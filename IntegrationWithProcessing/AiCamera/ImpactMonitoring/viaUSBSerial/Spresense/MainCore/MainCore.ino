/*
 *  Spresense.ino - Live Camera(JPEG) with Processing via USB serial.
 *  Copyright 2026 T.Hayakawa
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

#include <Camera.h>
#include <MP.h>
#include <USBSerial.h>

USBSerial UsbSerial;
const int sensor_core = 1;

// Anomaly flag used in JPEG metadata.
// 0: normal, 1: anomaly detected
class AnomalyFlag {
public:
  void update()
  {
    int8_t rcvid;
    uint32_t rcvdata;

    while (MP.Recv(&rcvid, &rcvdata, sensor_core) >= 0) {
      value_ = (uint8_t)(rcvdata & 0xFF);
    }
  }

  uint8_t read()
  {
    uint8_t current = value_;
    value_ = 0;
    return current;
  }

private:
  uint8_t value_ = 0;
};

AnomalyFlag anomaly_flag;

// Please change the serial setting for user environment
#define SERIAL_OBJECT   UsbSerial
#define SERIAL_BAUDRATE 921600

// Please select the display size
//int16_t width = CAM_IMGSIZE_QVGA_H,    height = CAM_IMGSIZE_QVGA_V;
//int16_t width = CAM_IMGSIZE_VGA_H,     height = CAM_IMGSIZE_VGA_V;
int16_t width = CAM_IMGSIZE_HD_H,      height = CAM_IMGSIZE_HD_V;
//int16_t width = CAM_IMGSIZE_QUADVGA_H, height = CAM_IMGSIZE_QUADVGA_V;
//int16_t width = CAM_IMGSIZE_FULLHD_H,  height = CAM_IMGSIZE_FULLHD_V;
//int16_t width = CAM_IMGSIZE_5M_H,      height = CAM_IMGSIZE_5M_V;
//int16_t width = CAM_IMGSIZE_3M_H,      height = CAM_IMGSIZE_3M_V;

void printError(enum CamErr err)
{
  switch (err)
    {
      case CAM_ERR_NO_DEVICE:
        puts("Error: No Device\n");
        break;
      case CAM_ERR_ILLEGAL_DEVERR:
        puts("Error: Illegal device error\n");
        break;
      case CAM_ERR_ALREADY_INITIALIZED:
        puts("Error: Already initialized\n");
        break;
      case CAM_ERR_NOT_INITIALIZED:
        puts("Error: Not initialized\n");
        break;
      case CAM_ERR_NOT_STILL_INITIALIZED:
        puts("Error: Still picture not initialized\n");
        break;
      case CAM_ERR_CANT_CREATE_THREAD:
        puts("Error: Failed to create thread\n");
        break;
      case CAM_ERR_INVALID_PARAM:
        puts("Error: Invalid parameter\n");
        break;
      case CAM_ERR_NO_MEMORY:
        puts("Error: No memory\n");
        break;
      case CAM_ERR_USR_INUSED:
        puts("Error: Buffer already in use\n");
        break;
      case CAM_ERR_NOT_PERMITTED:
        puts("Error: Operation not permitted\n");
        break;
      default:
        puts("Error: None...!\n");
        break;
    }
}

void errorLoop(int num)
{
  while (1) {
    for (int i = 0; i < num; i++) {
      ledOn(LED0);
      usleep(300 * 1000);
      ledOff(LED0);
      usleep(300 * 1000);
    }
    usleep(1000 * 1000);
  }
}

void setup()
{
  Serial.begin(115200);


  SERIAL_OBJECT.begin(SERIAL_BAUDRATE);
  Serial.println("Done!");

  Serial.println("Prepare camera");
  CamErr err = theCamera.begin();
  if (err != CAM_ERR_SUCCESS) {
    printf("theCamera.begin error = %d\n", (int)err);
    errorLoop(3);
  }

  puts("Enable HDR\n");
  err = theCamera.setHDR(CAM_HDR_MODE_AUTO);
  if (err != CAM_ERR_SUCCESS) printError(err);

  puts("Auto White Balance\n");
  err = theCamera.setAutoWhiteBalanceMode(CAM_WHITE_BALANCE_AUTO);
  if (err != CAM_ERR_SUCCESS) printError(err);

  puts("Auto Exposure + ISO\n");
  err = theCamera.setAutoExposure(true);
  if (err != CAM_ERR_SUCCESS) printError(err);

  err = theCamera.setAutoISOSensitivity(true);
  if (err != CAM_ERR_SUCCESS) printError(err);

  err = theCamera.setHDR(CAM_HDR_MODE_ON);
  if (err != CAM_ERR_SUCCESS) printError(err);

  err = theCamera.setColorEffect(CAM_COLOR_FX_VIVID);	
  if (err != CAM_ERR_SUCCESS) printError(err);

  err = theCamera.setExposureMetering(V4L2_EXPOSURE_METERING_CENTER_WEIGHTED);
  if (err != CAM_ERR_SUCCESS) printError(err);

  err = theCamera.setStillPictureImageFormat(
    width,                    
    height,
    CAM_IMAGE_PIX_FMT_JPG);
  if (err != CAM_ERR_SUCCESS)
    {
      printf("setStillPictureImageFormat error = %d\n", (int)err);
      errorLoop(5);
    }

  int ret = MP.begin(sensor_core);
  if (ret < 0) {
    printf("MP.begin error = %d\n", ret);
    errorLoop(2);
  }
  MP.RecvTimeout(MP_RECV_POLLING);

}

void loop()
{
  static int toggle = 0;
  static uint32_t prev_send_time = 0;

  anomaly_flag.update();

  // for debug
  CamImage img = theCamera.takePicture();
  if (img.isAvailable()) {
    (toggle++ & 1) ? ledOn(LED1) : ledOff(LED1);
    const uint8_t* jpg = img.getImgBuff();
    size_t jpg_size = img.getImgSize();
    uint8_t flag_to_send = anomaly_flag.read();
    send_jpeg(jpg, jpg_size, flag_to_send);
    uint32_t send_time = millis();
        uint32_t frame_time = (prev_send_time == 0) ? 0 : (send_time - prev_send_time);
    prev_send_time = send_time;
    printf("send_time:%lu, frame_time:%lu, flag:%u\n",
           (unsigned long)send_time,
           (unsigned long)frame_time,
           flag_to_send);
  }else{
    usleep(10*1000);
  }
}

int send_jpeg(const uint8_t* buffer, size_t size, uint8_t anomaly_flag)
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

  // Send 1byte anomaly metadata.
  SERIAL_OBJECT.write(anomaly_flag);

  // Send binary data
  size_t sent = 0;
  do {
    size_t s = SERIAL_OBJECT.write(&buffer[sent], size - sent);
    if ((int)s < 0) {
      return -1;
    }
    sent += s;
  } while (sent < size);

  SERIAL_OBJECT.write('E');  // Payload
  SERIAL_OBJECT.write('N');  // Payload
  SERIAL_OBJECT.write('D');  // Payload
  SERIAL_OBJECT.write('\n'); // Payload

  return 0;
}
