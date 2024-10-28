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
#include <DNNRT.h>
#include <SDHCI.h>
#include <BmpImage.h>

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

SDClass SD;
BmpImage bmp;
char fname[16];

#define OFFSET_X  (104)
#define OFFSET_Y  (4)
#define CLIP_WIDTH (112)
#define CLIP_HEIGHT (224)
#define DNN_WIDTH  (28)
#define DNN_HEIGHT  (28)

// ボタン押下時に呼ばれる割り込み関数
bool bButtonPressed = false;

// 画像データの保存用関数 
void saveGrayBmpImage(int swidth, int sheight, uint8_t* grayImg) 
{
  static int g_counter = 0;  // ファイル名につける追番
  sprintf(fname, "%03d.bmp", g_counter);

  // すでに画像ファイルがあったら削除
  if (SD.exists(fname)) SD.remove(fname);
  
  // ファイルを書き込みモードでオープン
  File bmpFile = SD.open(fname, FILE_WRITE);
  if (!bmpFile) {
    Serial.println("Fail to create file: " + String(fname));
    while(1);
  }
  
  // ビットマップ画像を生成
  bmp.begin(BmpImage::BMP_IMAGE_GRAY8, DNN_WIDTH, DNN_HEIGHT, grayImg); 
  // ビットマップを書き込み
  bmpFile.write(bmp.getBmpBuff(), bmp.getBmpSize());
  bmpFile.close();
  bmp.end();
  ++g_counter;
  // ファイル名を表示
  Serial.println("Saved an image as " + String(fname));
}

void CamCB(CamImage img)
{

  if (!img.isAvailable()) return;

  if (!Serial.available()) return;
  if (Serial.read() != '\n') return;
  if (bButtonPressed) return;

  // カメラ画像の切り抜きと縮小
  CamImage small;
  CamErr err = img.clipAndResizeImageByHW(small
                     , OFFSET_X, OFFSET_Y
                     , OFFSET_X + CLIP_WIDTH -1
                     , OFFSET_Y + CLIP_HEIGHT -1
                     , DNN_WIDTH, DNN_HEIGHT);
                     
  if (!small.isAvailable()) return;

  // 推論処理に変えて学習データ記録ルーチンに置き換え
  // 学習用データのモノクロ画像を生成
  uint16_t* imgbuf = (uint16_t*)small.getImgBuff();
  uint8_t grayImg[DNN_WIDTH*DNN_HEIGHT];
  for (int n = 0; n < DNN_WIDTH*DNN_HEIGHT; ++n) {
    grayImg[n] = (uint8_t)(((imgbuf[n] & 0xf000) >> 8) 
                         | ((imgbuf[n] & 0x00f0) >> 4));
  }

  Serial.println("Button Pressed");
  // 学習データを保存
  saveGrayBmpImage(DNN_WIDTH, DNN_HEIGHT, grayImg);
  bButtonPressed = true;

}

void setup()
{
  Serial.begin(115200);

  SERIAL_OBJECT.begin(SERIAL_BAUDRATE);
  Serial.println("Done!");

  while (!SD.begin()) {Serial.println("Insert SD card");}

  Serial.println("Prepare camera");
//  CamErr err = theCamera.begin();
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

  /* Start video stream */
  err = theCamera.startStreaming(true, CamCB);
  assert(err == CAM_ERR_SUCCESS);

  err = theCamera.setStillPictureImageFormat(
    width,                    
    height,
    CAM_IMAGE_PIX_FMT_JPG);
  if (err != CAM_ERR_SUCCESS)
    {
      puts("error!");
      exit(1);
    }

}

void loop()
{
  static int toggle = 0;
  static uint64_t base_time = 0;

  usleep(200*1000);

  // for debug
  CamImage imga = theCamera.takePicture();

  if (imga.isAvailable()) {

//    uint64_t now = millis();
//    base_time = now;
//    (toggle++ & 1) ? ledOn(LED1) : ledOff(LED1);
    send_jpeg(imga.getImgBuff(), imga.getImgSize());
//    uint64_t tdiff = millis() - now;
//    printf("time= %lld [ms] %.3lf [Mbps]\n", tdiff, (float)(imga.getImgSize() * 8) / tdiff / 1000);
  }else{
    usleep(10*1000);
  }

}

int send_jpeg(const uint8_t* buffer, size_t size)
{
  SERIAL_OBJECT.write('S'); // Payload
  SERIAL_OBJECT.write('P'); // Payload
  SERIAL_OBJECT.write('R'); // Payload
  SERIAL_OBJECT.write('S'); // Payload

  for(int i=0;i<16;i++){
      if (bButtonPressed) {
        SERIAL_OBJECT.write(fname[i]);
        printf("%c",fname[i]);
        fname[i] = '\0';
      }else{
        SERIAL_OBJECT.write('\0');
      }
  }

  if (bButtonPressed) bButtonPressed = false;

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

  SERIAL_OBJECT.write('E');  // Payload
  SERIAL_OBJECT.write('N');  // Payload
  SERIAL_OBJECT.write('D');  // Payload
  SERIAL_OBJECT.write('\n'); // Payload

  return 0;
}
