import hypermedia.net.*;
import controlP5.*;

import java.util.*;
import java.io.*;

UDP udp;
ControlP5 cp5;

final String TARGET_IP = "192.168.2.20";
final int TARGET_PORT = 10002;

String testMessage = "test_message";
OutputStream output;

void setup()
{
  // Please select the display size specified for a spresense sketch
  //size(320, 240);   // QVGA
  //size(640, 480);   // VGA
  //size(1280, 960);  // QUADVGA
  size(1280, 720);  // HD
  //size(1920, 1080); // FULLHD
  //size(2560, 1920); // 5M
  //size(2048, 1536); // 3M

  udp = new UDP(this, 10001);  
  udp.listen(true);

  sendUdpMessage();
}

void sendUdpMessage() {
  udp.send(testMessage, TARGET_IP, TARGET_PORT);
}

byte[] receivedData;
boolean receivedDataReady = false;
int receivedSize = 0;
int remainingBytes = 0;
int frameNumber = 0;

// previous short packet buffer
byte[] partialBuffer = new byte[0];

// for debug
int count = 0;
int baseTime = 0;
int now = 0;

void receive(byte[] data, String ip, int port) {

  // --- add previous fragment ---
  if (partialBuffer.length > 0) {
    data = concat(partialBuffer, data);
    partialBuffer = new byte[0];
  }

  // --- probably incomplete header ---
  if (data.length <= 8) {
    partialBuffer = data;
    println("Short packet buffered (len=" + data.length + ")");
    return;
  }

  // --- continuing frame ---
  if (remainingBytes > 0) {
    receivedData = concat(receivedData, data);
    remainingBytes -= data.length;
    if (remainingBytes <= 0) {
      receivedDataReady = true;
      println("Frame complete: frameNumber=" + frameNumber + ", size=" + receivedData.length + ", time=" + (now - baseTime) + "ms");
      baseTime = now;
    }    
  }
  // --- search for new frame ---
  else if (!findSync(data)) {
    // no SPRS found → keep last 8 bytes
    if (data.length > 8) {
      partialBuffer = Arrays.copyOfRange(data, data.length - 8, data.length);
    } else {
      partialBuffer = data;
    }
    return;
  }
}

boolean findSync(byte[] data)
{
  String syncWords = "0000";
  for (int i = 0; i < data.length; ) {
    syncWords = syncWords.substring(1);
    syncWords = syncWords + (char)data[i];
    i++;
    if (syncWords.equals("SPRS")) {
      if (i + 8 > data.length) {
        println("Not enough header bytes after SPRS, waiting next packet...");
        return false;
      }

      receivedSize = (data[i] & 0xff)
                   | ((data[i+1] & 0xff) << 8)
                   | ((data[i+2] & 0xff) << 16)
                   | ((data[i+3] & 0xff) << 24);
      i += 4;

      frameNumber = (data[i] & 0xff)
             | ((data[i+1] & 0xff) << 8)
             | ((data[i+2] & 0xff) << 16)
             | ((data[i+3] & 0xff) << 24);
      i += 4;

      println("receivedSize = ", receivedSize);
      println("frameNumber = ", frameNumber);
      receivedData = Arrays.copyOfRange(data, i, data.length);
      remainingBytes = receivedSize - (data.length - i);
      return true;
    }
  }
  return false;
}

void draw()
{
  if (!receivedDataReady) return;

  // Save data as a JPEG file and display it
  saveBytes(dataPath("sample.jpg"), receivedData);
  PImage img = loadImage(dataPath("sample.jpg"));
  println("x = ", img.width, "  y = ", img.height);
  image(img, 0, 0);

  receivedDataReady = false;
  receivedData = new byte[0];
  remainingBytes = 0;
}

void recover()
{
  udp.listen(false);
  delay(1000);
  udp.listen(true);
  delay(1000);
}
