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

#define OFFSET_X  (104)
#define OFFSET_Y  (4)
#define CLIP_WIDTH (112)
#define CLIP_HEIGHT (224)
#define DNN_WIDTH  (28)
#define DNN_HEIGHT  (28)

DNNVariable input(DNN_WIDTH*DNN_HEIGHT);
const char label[11] = {'0','1','2','3','4','5','6','7','8','9',' '};

uint8_t result_data = 10;
uint8_t result_prob = 0;

void CamCB(CamImage img)
{

  if (!img.isAvailable()) return;

  // カメラ画像の切り抜きと縮小
  CamImage small;
  CamErr err = img.clipAndResizeImageByHW(small
                     , OFFSET_X, OFFSET_Y
                     , OFFSET_X + CLIP_WIDTH -1
                     , OFFSET_Y + CLIP_HEIGHT -1
                     , DNN_WIDTH, DNN_HEIGHT);
                     
  if (!small.isAvailable()) return;

  // 認識用モノクロ画像をDNNVariableに設定
  uint16_t* imgbuf = (uint16_t*)small.getImgBuff();
  float *dnnbuf = input.data();
  for (int n = 0; n < DNN_HEIGHT*DNN_WIDTH; ++n) {
    // YUV422の輝度成分をモノクロ画像として利用
    // 学習済モデルの入力に合わせ0.0-1.0に正規化    
    dnnbuf[n] = (float)(((imgbuf[n] & 0xf000) >> 8) 
                      | ((imgbuf[n] & 0x00f0) >> 4))/255.;
  }
  // 推論の実行
  dnnrt.inputVariable(input, 0);
  dnnrt.forward();
  DNNVariable output = dnnrt.outputVariable(0);
  int index = output.maxIndex();
  
 // 推論結果の表示
  String gStrResult;
  if (index < 11) {
    result_data = index;
    result_prob = uint8_t (output[index] * 100);
    gStrResult = String(label[index]) 
        + String(":") + String(output[index]);
  } else {
    result_data = 10;
    result_prob = 0;
    gStrResult = String("Error");    
  }
  Serial.println(gStrResult);
}

void setup()
{
  Serial.begin(115200);

  SERIAL_OBJECT.begin(SERIAL_BAUDRATE);
  Serial.println("Done!");

  while (!SD.begin()) {Serial.println("Insert SD card");}
  // SDカードにある学習済モデルの読み込み
  File nnbfile = SD.open("number.nnb");

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
  SERIAL_OBJECT.write((char)0); // Payload
  SERIAL_OBJECT.write((char)result_data); // Payload
  SERIAL_OBJECT.write((char)0); // Payload
  SERIAL_OBJECT.write((char)result_prob); // Payload

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
