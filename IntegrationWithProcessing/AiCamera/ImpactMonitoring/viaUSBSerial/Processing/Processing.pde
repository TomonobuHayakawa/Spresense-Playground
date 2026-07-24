import java.util.Date;
import java.text.SimpleDateFormat;
import java.io.ByteArrayInputStream;
import java.awt.image.BufferedImage;
import javax.imageio.ImageIO;
import processing.serial.*;

Serial serial;
// Named rotation angles for readability.
final float R_0   = 0.0;
final float R_90  = HALF_PI;
final float R_180 = PI;
final float R_270 = PI + HALF_PI;

// Select one: R_0, R_90, R_180, R_270
final float ROTATION_ANGLE = R_0;
final boolean ROTATION_SWAP_WH = (abs(cos(ROTATION_ANGLE)) < 0.5);

// Base display size (must match camera output size setting)
final int FRAME_WIDTH = 1280;
final int FRAME_HEIGHT = 720;
final int MAX_FRAME_BYTES = 4 * 1024 * 1024;
final int FREEZE_SECONDS = 3;
final int FREEZE_BORDER_PX = 5;

// Please change the serial setting for user environment
final String SERIAL_PORTNAME = "COM99";
final int    SERIAL_BAUDRATE = 921600;

void settings()
{
  // For 90/270 rotation, swap display frame width and height.
  if (ROTATION_SWAP_WH) {
    size(FRAME_HEIGHT, FRAME_WIDTH);
  } else {
    size(FRAME_WIDTH, FRAME_HEIGHT);
  }
}

void setup()
{
  serial = new Serial(this, SERIAL_PORTNAME, SERIAL_BAUDRATE);
}

void draw()
{
   recieve_data();
}

// Sync word finder
boolean find_sync(int timeout)
{
  String sync_words = "0000";
  int expire = millis() + timeout;

  while (true) {
    if (serial.available() > 0) {
      sync_words = sync_words.substring(1);
      sync_words = sync_words + (char)serial.read();
      if(sync_words.equals("SPRS")){
        return true;
      }
    } else {
      delay(50);
    }

    if (timeout > 0) {
      if (millis() > expire) {
        return false;
      }
    }
  }
}

// End code checker.
boolean find_end()
{
  String end_words = new String();
  for(int i= 0;i<4;i++){
    if (serial.available() > 0) {
      end_words += (char)serial.read();
    }
  }
  if(!end_words.equals("END\n")){
    println("Do not find end code.");
    return false;
  }
  return true;
}

PImage displayed_img;
int freeze_until = 0;
int displayed_data_ms = -1;
int displayed_data_flag = 0;

PImage decode_jpeg_from_bytes(byte[] data)
{
  try {
    BufferedImage bi = ImageIO.read(new ByteArrayInputStream(data));
    if (bi == null) {
      return null;
    }

    int w = bi.getWidth();
    int h = bi.getHeight();
    PImage img = createImage(w, h, RGB);
    img.loadPixels();
    bi.getRGB(0, 0, w, h, img.pixels, 0, w);
    img.updatePixels();
    return img;
  } catch (Exception e) {
    println("jpeg decode fail");
    return null;
  }
}

void recieve_data()
{
  int size = 0;
  int anomaly_flag = 0;
  boolean start_freeze = false;

  // Search sync words
  if (find_sync(3000)) {
    // Receive a binary data size in 4byte
    size = serial.read()<<24 | serial.read()<<16 | serial.read()<<8 | serial.read();
    anomaly_flag = serial.read();
  } else {
    println("recover1");
    recover();
    return;
  }

  // illegal size
  if (size <= 0 || size > MAX_FRAME_BYTES) {
    println("illegal size=", size);
    serial.clear();
    return;
  }

  // Receive binary data
  byte [] data = new byte[size];
  int timeout = millis() + 5000;
  for (int i = 0; i < size; ) {
    if (serial.available() > 0) {
      data[i] = (byte)serial.read();
      i++;
    } else {
      if (millis() > timeout) {
        println("recover2");
        recover();
        return;
      }
      delay(1);
    }
  }

  if (!find_end()) {
    recover();
    return;
  }

  if (millis() >= freeze_until) {
    PImage img = decode_jpeg_from_bytes(data);
    if (img == null) {
      return;
    }
    displayed_img = img;
    displayed_data_ms = millis();
    displayed_data_flag = anomaly_flag;
    if (anomaly_flag == 1) {
      start_freeze = true;
    }
  }

  if (displayed_img == null) {
    return;
  }

  drawRotatedImage(displayed_img);
  println("display_ms=", displayed_data_ms, " flag=", displayed_data_flag, " age_ms=", (millis() - displayed_data_ms));
  if (start_freeze || millis() < freeze_until) {
    drawFreezeBorder();
  }

  if (start_freeze) {
    freeze_until = millis() + FREEZE_SECONDS * 1000;
  }
}

void drawRotatedImage(PImage img)
{
  pushMatrix();
  imageMode(CENTER);
  translate(width / 2.0, height / 2.0);
  rotate(ROTATION_ANGLE);
  image(img, 0, 0, img.width, img.height);
  popMatrix();
  imageMode(CORNER);
}

void drawFreezeBorder()
{
  noFill();
  stroke(255, 0, 0);
  strokeWeight(FREEZE_BORDER_PX);
  rectMode(CORNER);
  rect(0, 0, width, height);
}

//
// recover any error
//
void recover()
{
  if (serial != null) {
    serial.clear();
    serial.stop();
  }
  try {
    serial = new Serial(this, SERIAL_PORTNAME, SERIAL_BAUDRATE);
  } catch (Exception e) {
    serial = null;
  }
}

String timestamp()
{
  Date date = new Date();
  SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMdd-HHmmss-SSS");
  return sdf.format(date);
}
