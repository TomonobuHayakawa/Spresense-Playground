#define USE_OLED
//#define DATA_COLLECTION

#ifdef DATA_COLLECTION
//#define CAPTURE_COLLECTION
#define DNN_COLLECTION
#endif

#include <Camera.h>
#include <SoftwareSerial.h>
#include <DNNRT.h>
#include <Flash.h>

#ifdef DATA_COLLECTION
#include <SDHCI.h>
SDClass SD;
#endif

#ifdef USE_OLED
#include <Wire.h>
#include "U8g2lib.h"
U8G2_SSD1327_EA_W128128_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
#endif

#define bootPin 7

#define rxPin 1
#define txPin 0

SoftwareSerial mySerial = SoftwareSerial(rxPin, txPin);

const int16_t width = CAM_IMGSIZE_QQVGA_H, height = CAM_IMGSIZE_QQVGA_V;

#define DNN_OFFSET_X (36)
#define DNN_OFFSET_Y (0)
#define DNN_CLIP_WIDTH (56)
#define DNN_CLIP_HEIGHT (112)
#define DNN_WIDTH (28)
#define DNN_HEIGHT (28)

DNNRT dnnrt;
DNNVariable dnn_input(DNN_WIDTH*DNN_HEIGHT);
const char label[11] = {'0','1','2','3','4','5','6','7','8','9',' '};

#define DISP_WIDTH 128
#define DISP_HEIGHT 120

static uint8_t display_buffer[DISP_HEIGHT][DISP_WIDTH / 8];

void draw(uint8_t* in, int height, int width)
{
  int disp_offset = (width - DISP_WIDTH) / 2;

/*  printf("height=%d\n", height);
  printf("width=%d\n", width);
  printf("disp_offset=%d\n", disp_offset);*/

  for (int h = 0; h < height; h++) {
    for (int w = 0; w < width / 8; w++) {
      if ((w < disp_offset / 8) || (w >= (DISP_WIDTH + disp_offset) / 8)){
        in+=8;
        continue;
      }
      display_buffer[h][w - (disp_offset / 8)] = 0;
      for (int i = 0; i < 8; i++) {
        display_buffer[h][w - (disp_offset / 8)] |= ((*in > 0x60) ? 1 : 0) << i;
        in++;
      }
    }
  }
  u8g2.clearBuffer();
  u8g2.setDrawColor(1);
  u8g2.setBitmapMode(false);
  /*実はこのデバイス高さ方向は96サンプルしか表示できない。*/
  u8g2.drawXBMP(0, 0, 128, 124, (uint8_t*)display_buffer);
  u8g2.setDrawColor(2);
  /*実はこのデバイス高さ方向は96サンプルしか表示できない。*/
  u8g2.drawFrame(DNN_OFFSET_X, DNN_OFFSET_Y, DNN_CLIP_WIDTH, 96);
  u8g2.sendBuffer();

}

#ifdef DATA_COLLECTION
void save(uint8_t* buf, int height, int width)
{
  char fname[16];
  static int counter = 0;
  sprintf(fname, "%03d.pgm", counter);
  printf("%03d.pgm\n", counter);

  if (SD.exists(fname)) SD.remove(fname);
  File myFile = SD.open(fname, FILE_WRITE);

  char header[64];
  sprintf(header,"P5\n%d %d\n%d\n",width,height,255);
  myFile.write(header);

  myFile.write(buf, height * width);
  myFile.close();

  counter++;
}
#endif

String recognize(uint8_t* buf, int height, int width)
{
  int disp_offset = (width - DISP_WIDTH) / 2;

  float *dnnbuf = dnn_input.data();

  for (int h = 0; h < height; h+=4) {
    if ((h < DNN_OFFSET_Y ) || (h >= (DNN_OFFSET_Y + DNN_CLIP_HEIGHT))){
      buf+=(width*4);
      continue;
    }
    for (int w = 0; w < width; w+=2) {
      if ((w < DNN_OFFSET_X + disp_offset) || (w >= (DNN_OFFSET_X + disp_offset + DNN_CLIP_WIDTH))){
        buf+=2;
        continue;
      }
      *dnnbuf = (float)*buf/255./8.;
      *dnnbuf += (float)*(buf+1)/255./8.;
      *dnnbuf += (float)*(buf+width)/255./8.;
      *dnnbuf += (float)*(buf+width+1)/255./8.;
      *dnnbuf += (float)*(buf+width*2)/255./8.;
      *dnnbuf += (float)*(buf+width*2+1)/255./8.;
      *dnnbuf += (float)*(buf+width*3)/255./8.;
      *dnnbuf += (float)*(buf+width*3+1)/255./8.;
       buf+=2;
       dnnbuf++;
    }
    buf+=width*3;
  }

  uint8_t dnn_img[DNN_WIDTH*28];

  float* img_f = dnn_input.data();
  for(int i=0;i<DNN_WIDTH*DNN_HEIGHT;i++,img_f++){
    dnn_img[i]=(uint8_t)(*img_f*255);
  }

#ifdef DNN_COLLECTION
  save(dnn_img,DNN_WIDTH,DNN_HEIGHT);
#endif

  dnnrt.inputVariable (dnn_input, 0);
  dnnrt.forward();
  DNNVariable output = dnnrt.outputVariable(0);
  int index = output.maxIndex();
  
  String gStrResult;
  if (index < 11) {
    gStrResult = String(label[index]) + String(":") + String(output[index]);
  } else {
    gStrResult = String("Error");    
  }

  return gStrResult;

}

void setup(void)
{
  pinMode(bootPin, INPUT);

  Serial.begin(9600);
  Serial.println("Spresense booted!");

#ifdef DATA_COLLECTION
  while (!SD.begin()) {
    ; /* wait until SD card is mounted. */
  }
#endif

  // SDカードかFlashにある学習済モデルの読み込み
  File nnbfile = Flash.open("model.nnb");
//  File nnbfile = SD.open("model.nnb");
  // 学習済モデルでDNNRTを開始
  int ret = dnnrt.begin(nnbfile);
  if (ret < 0) {
    Serial.println("dnnrt.begin failed" + String(ret));
    return;
  }
  nnbfile.close();

  while (digitalRead(bootPin) == HIGH);

  sleep(1);
  CamErr err = theCamera.begin();
  assert(err == CAM_ERR_SUCCESS);

  err = theCamera.setAutoWhiteBalanceMode(CAM_WHITE_BALANCE_AUTO);
  assert(err == CAM_ERR_SUCCESS);

  err = theCamera.setStillPictureImageFormat(
    width,
    height,
    CAM_IMAGE_PIX_FMT_YUV422);
  if (err != CAM_ERR_SUCCESS) {
    puts("error!");
    exit(1);
  }

  u8g2.begin();

  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  mySerial.begin(9600);

}

void loop(void)
{
  CamImage img = theCamera.takePicture();
  if (img.isAvailable()) {
    img.convertPixFormat(CAM_IMAGE_PIX_FMT_GRAY);
    draw((uint8_t*)img.getImgBuff(), img.getHeight(), img.getWidth());
    String result = recognize((uint8_t*)img.getImgBuff(), img.getHeight(), img.getWidth());
//    Serial.println(result);
    mySerial.println(result);

#ifdef CAPTURE_COLLECTION
    save((uint8_t*)img.getImgBuff(), img.getHeight(), img.getWidth());
#endif

  } else {
    usleep(10 * 1000);
  }
}
