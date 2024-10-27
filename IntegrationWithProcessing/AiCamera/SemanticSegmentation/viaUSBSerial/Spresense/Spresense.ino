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
DNNRT dnnrt;

#define OFFSET_X  (48)
#define OFFSET_Y  (6)
#define CLIP_WIDTH  (224)
#define CLIP_HEIGHT  (224)
#define DNN_WIDTH  (28)
#define DNN_HEIGHT  (28)

// RGBの画像を入力
DNNVariable input(DNN_WIDTH*DNN_HEIGHT*3);  

uint16_t sx, swidth, sy, sheight;

void CamCB(CamImage img)
{

  if (!img.isAvailable()) return;

  // 画像の切り出しと縮小
  CamImage small; 
  CamErr camErr = img.clipAndResizeImageByHW(small
            ,OFFSET_X ,OFFSET_Y 
            ,OFFSET_X+CLIP_WIDTH-1 ,OFFSET_Y+CLIP_HEIGHT-1 
            ,DNN_WIDTH ,DNN_HEIGHT);
  if (!small.isAvailable()) return;

  // 画像をYUVからRGB565に変換
  small.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565); 
  uint16_t* sbuf = (uint16_t*)small.getImgBuff();

  // RGBのピクセルをフレームに分割
  float* fbuf_r = input.data();
  float* fbuf_g = fbuf_r + DNN_WIDTH*DNN_HEIGHT;
  float* fbuf_b = fbuf_g + DNN_WIDTH*DNN_HEIGHT;
  for (int i = 0; i < DNN_WIDTH*DNN_HEIGHT; ++i) {
   fbuf_r[i] = (float)((sbuf[i] >> 11) & 0x1F)/31.0; // 0x1F = 31
   fbuf_g[i] = (float)((sbuf[i] >>  5) & 0x3F)/63.0; // 0x3F = 64
   fbuf_b[i] = (float)((sbuf[i])       & 0x1F)/31.0; // 0x1F = 31
  }
  
  // 推論を実行
  dnnrt.inputVariable(input, 0);
  dnnrt.forward();
  DNNVariable output = dnnrt.outputVariable(0); 
 
  // DNNRTの結果をLCDに出力するために画像化
  static uint16_t result_buf[DNN_WIDTH*DNN_HEIGHT];
  for (int i = 0; i < DNN_WIDTH * DNN_HEIGHT; ++i) {
    uint16_t value = output[i] * 0x3F; // 6bit
    if (value > 0x3F) value = 0x3F;
    result_buf[i] = (value << 5);  // Only Green
  }
  
  // 認識対象の横幅と横方向座標を取得
  bool err;
  int16_t s_sx, s_width;
  err = get_sx_and_width_of_region(output, DNN_WIDTH, DNN_HEIGHT, &s_sx, &s_width);
  
  // 認識対象の縦幅と縦方向座標を取得
  int16_t s_sy, s_height;
  sx = swidth = sy = sheight = 0;
  err = get_sy_and_height_of_region(output, DNN_WIDTH, DNN_HEIGHT, &s_sy, &s_height);
  if (!err) {
    Serial.println("detection error");
    return;
  }
  
  // 何も検出できなかった
  if (s_width == 0 || s_height == 0) {
    Serial.println("no detection");
    return;
  }
  
  // 認証対象のボックスと座標をカメラ画像にあわせて拡大
  sx = s_sx * (CLIP_WIDTH/DNN_WIDTH) + OFFSET_X;
  swidth = s_width * (CLIP_WIDTH/DNN_WIDTH);
  sy = s_sy * (CLIP_HEIGHT/DNN_HEIGHT) + OFFSET_Y;
  sheight = s_height * (CLIP_HEIGHT/DNN_HEIGHT);

}

void setup()
{
  Serial.begin(115200);

  SERIAL_OBJECT.begin(SERIAL_BAUDRATE);
  Serial.println("Done!");

  while (!SD.begin()) {Serial.println("Insert SD card");}
  // SDカードにある学習済モデルの読み込み
  File nnbfile = SD.open("segmentation.nnb");

  if(nnbfile == 0){ puts("NNB Open Error!"); exit(1); }
  // 学習済モデルでDNNRTを開始
  dnnrt.begin(nnbfile);

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
  SERIAL_OBJECT.write((char)((sx >> 8) & 0xff)); // Payload
  SERIAL_OBJECT.write((char)(sx & 0xff)); // Payload
  SERIAL_OBJECT.write((char)((sy >> 8) & 0xff)); // Payload
  SERIAL_OBJECT.write((char)(sy & 0xff)); // Payload
  SERIAL_OBJECT.write((char)((swidth >> 8) & 0xff)); // Payload
  SERIAL_OBJECT.write((char)(swidth & 0xff)); // Payload
  SERIAL_OBJECT.write((char)((sheight >> 8) & 0xff)); // Payload
  SERIAL_OBJECT.write((char)(sheight & 0xff)); // Payload

  printf("sx=%d,swidth=%d,sy=%d,sheight=%d\n",sx,swidth,sy,sheight);

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
