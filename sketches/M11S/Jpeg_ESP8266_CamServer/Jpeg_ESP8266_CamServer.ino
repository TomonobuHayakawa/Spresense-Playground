/*
 * M11S Jpeg shot & ESP8266 on Camera Server 
 * Copyright 2019 Tomonobu Hayakawa
 *
 * Modify from https://github.com/YoshinoTaro/ESP8266ATLib-for-Spresense by Yoshino Taro
 * 
 * This example demostrate how to make a simple serveillance camera using ESP8266ATLib.
 * Please note that this library is made for Spresense ESP9266 Wi-Fi Add-on board.
 * 
 * After flushing this example to Spresense, 
 * access an IP address with cam.jpg like http://xxx.xxx.xxx.xxx/cam.jpg by a browser, 
 * then you can see a captured image on the page
 * 
 * This example code is under LGPL v2.1 
 * (because Arduino library of Spresense is under LGPL v2.1)
 */

#include <SPI.h>
#include "ESP8266ATLib.h"
#include "asmp/mpshm.h"

#define SAP_MODE
//#define BAUDRATE 115200
#define BAUDRATE 1152000

#define BUFSIZE 2048

#ifdef SAP_MODE
#define SSID "SprESP8266AP"
#define PASS "123123123"
#else
#define SSID "your ssid"
#define PASS "your passwd"
#endif

#define FRAME_SIZE          (512 * 1024)            // 512KiB for potential FHD sized image

#define CHUNK_SIZE      (32* 1024)                  // 32 KiB (Note : DMA size or under)
#define CMD_SIZE        (8)                         // CMD packet length
#define RES_SIZE        (8)                         // Response packet length

#define CMD_READ_RES    (0x00)                      // CMD = Read Response
#define CMD_GET_JPEG    (0x01)                      // CMD = Get the JPEG file

#define RES_ACK         (0x01)
#define RES_CMDERR      (0xF1)
#define RES_SYSERR      (0xF2)
#define RES_CSUMERR     (0xF6)

#define RES_RETRY_TIME  (3000)                      // Retry total time(ms)
#define RES_RETRY_COUNT (500)                       // Retry counter for Response

uint8_t tx_buffer[CHUNK_SIZE];
uint8_t rx_buffer[CHUNK_SIZE];
uint8_t* imgbuf = NULL;

mpshm_t shm;


/***----- M11S Control -----------------------------------------------------***/
void send_cmd(uint8_t cmd, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4,
              uint8_t p5, uint8_t p6)
{
  uint32_t i;

  tx_buffer[0] = cmd;
  tx_buffer[1] = p1;
  tx_buffer[2] = p2;
  tx_buffer[3] = p3;
  tx_buffer[4] = p4;
  tx_buffer[5] = p5;
  tx_buffer[6] = p6;

  for (tx_buffer[CMD_SIZE - 1] = i = 0; i < (CMD_SIZE - 1); i++) // CS(Check sum)
  {
    tx_buffer[CMD_SIZE - 1] += tx_buffer[i];
  }
  tx_buffer[CMD_SIZE - 1] ^= 0xFF;

  SPI5.transfer(tx_buffer, CMD_SIZE);

  printf("%x,%x,%x,%x\n",tx_buffer[0],tx_buffer[1],tx_buffer[2],tx_buffer[3]);
  
  return;
}

static int recv_res(void)
{
  uint32_t retry;
  uint8_t sum;
  uint32_t i;

  printf("wait response\n");

  for (retry = 0; retry < RES_RETRY_COUNT; retry++)
  {
    SPI5.transfer(tx_buffer, CMD_SIZE);

    if (tx_buffer[0] != 0xFF)                  // 0xFF = Retry
    {

      for (sum = i = 0; i < (RES_SIZE - 1); i++) // CS(Check sum)
      {
        sum += tx_buffer[i];
      }
      sum ^= 0xFF;

      if (tx_buffer[0] == RES_ACK) puts("ACK!!!");

      if ((tx_buffer[0] == RES_ACK)
          && (sum == tx_buffer[RES_SIZE - 1]))
      {
        return 0;                         // Normal end
      }

      printf("Not ACK\n");                // retry
    }
    delay(RES_RETRY_TIME / RES_RETRY_COUNT);
  }

  printf("Response retry over\n");

ERR_EXIT:
  return -1;
}

static int recv_data(void *buffer_addr, uint32_t buffer_size, uint32_t *pData_size)
{
  uint32_t chunk_number, read_length;

  chunk_number = 1;
  *pData_size = 0;

  for (uint32_t i = 0; i < chunk_number; i++)
  {
    recv_res();

    if (i != (tx_buffer[1] | ((tx_buffer[2]) << 8)))
    {
      printf("Abnormal chunk number\n");
      goto ERR_EXIT;
    }

    if (i == 0)                                       // 1st time?
    {
      *pData_size = tx_buffer[3] | (tx_buffer[4] << 8) | (tx_buffer[5] << 16)
                    | (tx_buffer[6] << 24);
      chunk_number = (*pData_size + (CHUNK_SIZE - RES_SIZE - 1))
                     / (CHUNK_SIZE - RES_SIZE);

      printf("Data size %d\n", *pData_size);
      printf("Total number of chunk = %d\n", chunk_number);

      if (buffer_size < *pData_size)
      {
        printf("Not enough memory\n");
        goto ERR_EXIT;
      }

      if (*pData_size >= (CHUNK_SIZE - RES_SIZE))
      {
        read_length = CHUNK_SIZE - RES_SIZE;
      }
      else
      {
        read_length = *pData_size;
      }
    }
    else
    {
      read_length = tx_buffer[3] | (tx_buffer[4] << 8) | (tx_buffer[5] << 16)
                    | (tx_buffer[6] << 24);
    }

    printf("chunk number = %d\n", i);
    printf("read_length = %d\n", read_length);

    if (read_length > (CHUNK_SIZE - RES_SIZE))
    {
      goto ERR_EXIT;
    }

    SPI5.transfer(rx_buffer, read_length);

    memcpy(buffer_addr, rx_buffer, read_length);
    buffer_addr += read_length;
  }

  return 0;

ERR_EXIT:
  return -1;
}


/***----- setup -----------------------------------------------------------***/
void setup() {

  /* Start the SPI library */
  SPI5.begin();

  /* Configure the SPI port */
  SPI5.beginTransaction(SPISettings(5200000, MSBFIRST, SPI_MODE1));

  /* M11S Power On */
  pinMode(21, OUTPUT);
  digitalWrite(21, LOW);
  digitalWrite(21, HIGH);
  puts("M11S Power On");

  /* wait for M11S boot */
  sleep(40);

  /* Allocate shared RAM */
  int ret = mpshm_init(&shm, 0, FRAME_SIZE);
  imgbuf = mpshm_attach(&shm, 0);

  /* Start ESP8266 library */
  esp8266at.begin(BAUDRATE);

#ifdef SAP_MODE
  esp8266at.espStartAP(SSID, PASS);
#else
  esp8266at.espConnectAP(SSID, PASS);
#endif
  digitalWrite(LED0, HIGH);
  esp8266at.setupTcpServer("80");
  digitalWrite(LED1, HIGH);
  
  Serial.println();
  Serial.println();
  Serial.println("---------------------------------------------"); 
  Serial.println("Try to access the address below.");
  Serial.println("http://" + esp8266at.getLocalIP() + "/cam.jpg");
  Serial.println(); 
  Serial.println("You can see a captured picture on the page");
  Serial.println("---------------------------------------------"); 
  Serial.println();

  // esp8266at.setWaitTime(100);

}

/***----- loop ------------------------------------------------------------***/
void loop() {

  digitalWrite(LED2, LOW);
  String linkID = "";
  String s = esp8266at.espListenToClient(&linkID);
  boolean result = false;
  int32_t data_size;

  if (!(s.startsWith("+IPD") && s.indexOf("HTTP/1"))) return; 
  if (s.indexOf("GET") < 0) return;  // only GET acceptable
  // linkID = esp8266at.extractLinkID(s);
  Serial.println(s);

  if (s.indexOf("cam.jpg") != -1) {

    digitalWrite(LED2, HIGH);

    puts("Taking a picture");
    Serial.println("Taking a picture");
    send_cmd(CMD_GET_JPEG, 1, 0, 0, 0, 0, 0);
    recv_data(imgbuf, FRAME_SIZE, &data_size);
//  digitalWrite(21, LOW);

    puts("Taking a picture end");

    Serial.println("Image Size: " + String(data_size));
    delay(100);

    String msg = "HTTP/1.1 200 OK\r\n";
    msg += "Content-Type: image/jpeg\r\n";
    msg += "Content-Length: ";
    msg += String(data_size) + "\r\n";
    msg += "\r\n";
    Serial.print(msg);
    esp8266at.sendMessageToClient(linkID, msg);

    for (; data_size > 0; imgbuf += BUFSIZE, data_size -= BUFSIZE) {
      uint16_t sendDataSize = min(data_size, BUFSIZE);
      Serial.println("data size: " + String(sendDataSize));
      result = esp8266at.sendBinaryToClient(linkID, imgbuf, sendDataSize);
      if (!result) {
        Serial.println("Send data is fault");
        break;
      }
    }

    digitalWrite(LED2, LOW);
  }
  else {
    String uri = "http://" + esp8266at.getLocalIP() + "/cam.jpg";
    String msg = "HTTP/1.1 200 OK\r\n";
    msg += "Content-Type: text/html\r\n";
    msg += "\r\n";
    Serial.print(msg);
    esp8266at.sendMessageToClient(linkID, msg);
    msg ="<html>please access <a href="  + uri + ">" + uri + "</a></html>\r\n";
    esp8266at.sendMessageToClient(linkID, msg);
  }

  Serial.println("Connection closed: " + linkID);
  esp8266at.closeClientConnection(linkID);
  delay(100);
}
