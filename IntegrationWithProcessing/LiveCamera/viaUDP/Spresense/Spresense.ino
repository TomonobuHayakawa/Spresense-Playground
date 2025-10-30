/*
 *  Spresense.ino - Live Camera with Processing via WiFi UDP.
 *  Copyright 2025 T.Hayakawa
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
#include <TelitWiFi.h>
#include "config.h"

#define TXBUFFER_SIZE 1400  // UDP送信時の分割サイズ（MTU対策）

/* --------------------------------------------------------------------
 * variables
 * --------------------------------------------------------------------*/
TelitWiFi gs2200;
TWIFI_Params gsparams;
TWIFI_Adress adrs;
int server_cid = ATCMD_INVALID_CID;

// Please select the display size
//int16_t width = CAM_IMGSIZE_QVGA_H,    height = CAM_IMGSIZE_QVGA_V;
//int16_t width = CAM_IMGSIZE_VGA_H,     height = CAM_IMGSIZE_VGA_V;
//int16_t width = CAM_IMGSIZE_HD_H,      height = CAM_IMGSIZE_HD_V;
int16_t width = CAM_IMGSIZE_QUADVGA_H, height = CAM_IMGSIZE_QUADVGA_V;
//int16_t width = CAM_IMGSIZE_FULLHD_H,  height = CAM_IMGSIZE_FULLHD_V;
//int16_t width = CAM_IMGSIZE_5M_H,      height = CAM_IMGSIZE_5M_V;
//int16_t width = CAM_IMGSIZE_3M_H,      height = CAM_IMGSIZE_3M_V;

/* --------------------------------------------------------------------
 * setup
 *--------------------------------------------------------------------*/
void setup()
{
  Serial.begin(115200);
  ConsoleLog("=== Spresense + GS2200 (TelitWiFi) UDP Camera Sender ===");

  /* Initialize SPI access of GS2200 */
  Init_GS2200_SPI_type(iS110B_TypeA);
  /* Initialize AT Command Library Buffer */
  gsparams.mode = ATCMD_MODE_STATION;
  gsparams.psave = ATCMD_PSAVE_DEFAULT;

#ifdef USE_DHCP
  if (gs2200.begin(gsparams)) {
#else
  strcpy(adrs.device, DEVICE_IP);
  strcpy(adrs.gateway, GATWAY_IP);
  strcpy(adrs.subnet, SUBNET_MASK);
  if (gs2200.begin(gsparams, false, adrs)) {
#endif
    ConsoleLog("GS2200 Initialization Fails");
    while (1);
  }

  // --- アクセスポイント接続 ---
  if (gs2200.activate_station(AP_SSID, PASSPHRASE)) {
    ConsoleLog("Association Fails");
    while (1);
  }

  ConsoleLog("Wi-Fi connected successfully.");
  ConsoleLog("Start UDP Client...");

  // --- UDP接続開始 ---
  while (1) {
    server_cid = gs2200.connectUDP(UDPSRVR_IP, UDPSRVR_PORT, LocalPort);
    ConsolePrintf("server_cid: %d\r\n", server_cid);
    if (server_cid != ATCMD_INVALID_CID) {
      ConsoleLog("UDP Connection OK!");
      break;
    }
    delay(1000);
  }

  // --- カメラ初期化 ---
  Serial.println("Prepare camera");
  CamErr err = theCamera.begin();
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

  err = theCamera.setStillPictureImageFormat(
    width,                    
    height,
    CAM_IMAGE_PIX_FMT_JPG, 5);
  if (err != CAM_ERR_SUCCESS)
    {
      puts("error!");
      exit(1);
    }
}

/* --------------------------------------------------------------------
 * loop
 *--------------------------------------------------------------------*/
void loop()
{
  CamImage img = theCamera.takePicture();
  if (!img.isAvailable()) {
    delay(10);
    return;
  }

  uint8_t* buf = img.getImgBuff();
  size_t size = img.getImgSize();

  send_frame(buf, size);
}

/* --------------------------------------------------------------------
 * JPEGデータをGS2200経由でUDP送信
 * --------------------------------------------------------------------*/
void send_frame(uint8_t* ptr, size_t size)
{
  ConsolePrintf("Send Frame: %u bytes\n", (unsigned int)size);

  static uint32_t frame_no = 0;

  // --- Header (SPRS + size + frame_no = 12バイト固定) ---
  uint8_t header[12];
  header[0] = 'S';
  header[1] = 'P';
  header[2] = 'R';
  header[3] = 'S';

  header[7] = (size >> 24) & 0xFF;
  header[6] = (size >> 16) & 0xFF;
  header[5] = (size >> 8) & 0xFF;
  header[4] = (size) & 0xFF;

  header[11]  = (frame_no >> 24) & 0xFF;
  header[10]  = (frame_no >> 16) & 0xFF;
  header[9] = (frame_no >> 8)  & 0xFF;
  header[8] = (frame_no) & 0xFF;

  printf("frame no=%d\n",frame_no);
  printf("size = %d,%d,%d,%d\n",header[7],header[6],header[5],header[4]);

  // --- Send header once (12 bytes) ---
  if (!gs2200.write(server_cid, header, sizeof(header))) {
    ConsoleLog("UDP write header failed");
    return;
  }

  size_t remaining = size;
  while (remaining > 0) {
    int write_size = (remaining > TXBUFFER_SIZE) ? TXBUFFER_SIZE : remaining;
    bool ret = gs2200.write(server_cid, (const uint8_t*)ptr, write_size);
    if (!ret) {
      ConsoleLog("UDP write failed");
      return;
    }
    ptr += write_size;
    remaining -= write_size;
  }

  // --- Footer ---
  const char end[] = "END\n";
  gs2200.write(server_cid, (const uint8_t*)end, sizeof(end) - 1);
  frame_no++;

  usleep(10*1000);
  ConsoleLog("Frame sent successfully");

}

