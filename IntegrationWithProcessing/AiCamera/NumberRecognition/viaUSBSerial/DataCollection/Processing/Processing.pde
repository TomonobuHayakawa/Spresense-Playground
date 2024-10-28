import java.util.Date;
import java.text.SimpleDateFormat;
import processing.serial.*;

Serial serial;

// Please change the serial setting for user environment
final String SERIAL_PORTNAME = "COM105";
final int    SERIAL_BAUDRATE = 921600;

PFont myFont;

void setup()
{
  // Please select the display size specified for a spresense sketch
  //size(320, 240);   // QVGA
  size(640, 480);   // VGA
  //size(1280, 960);  // QUADVGA
  // size(1280, 720);  // HD
  //size(1920, 1080); // FULLHD
  //size(2560, 1920); // 5M
  //size(2048, 1536); // 3M

  myFont = createFont("Arial", 30);
  textFont(myFont); 

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
      if (sync_words.equals("SPRS")) {
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
  for (int i= 0; i<4; i++) {
    if (serial.available() > 0) {
      end_words += (char)serial.read();
    }
  }
  if (!end_words.equals("END\n")) {
    println("Do not find end code.");
    return false;
  }
  return true;
}

// for debug
int count = 0;
int base_time = 0;

//char fname[16];
String fname = "";

void recieve_data()
{
  int size = 0;

  // Search sync words
  if (find_sync(3000)) {

    String tmp = "";

    for (int i = 0; i < 16; i++) {
      tmp += (char)serial.read();
    }

//    if ((tmp.charAt(0) != 0) && (fname.length() != 0)) {
    if ((tmp.charAt(0) != 0)) {
      fname = tmp;
//      println("fname = " + fname);
      println("fname = " + tmp);
    }
    tmp = "";

    // Receive a binary data size in 4byte
    size = serial.read()<<24 | serial.read()<<16 | serial.read()<<8 | serial.read();
  } else {
    println("recover1");
    recover();
    return;
  }

  // illegal size
  if (size <= 0) {
    serial.clear();
    return;
  }

  // for debug
  int now = millis();
//  println(count++, ": size=", size, "time=", now - base_time, "[ms]");
  base_time = now;

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
      delay(50);
    }
  }

  if (!find_end()) return;

  int OFFSET_X = (104*2);
  int OFFSET_Y = (4*2);
  int CLIP_WIDTH = (112*2);
  int CLIP_HEIGHT = (224*2);

  // Save data as a JPEG file and display it
  saveBytes(dataPath("sample.jpg"), data);
  PImage img = loadImage(dataPath("sample.jpg"));
  //  println("x = ",img.width,"  y = ",img.height);
  image(img, 0, 0);

  stroke( #ff0000 );
  strokeWeight( 2 );
  line( OFFSET_X, OFFSET_Y, OFFSET_X+CLIP_WIDTH, OFFSET_Y);
  line( OFFSET_X, OFFSET_Y, OFFSET_X, OFFSET_Y+CLIP_HEIGHT);
  line( OFFSET_X+CLIP_WIDTH, OFFSET_Y, OFFSET_X+CLIP_WIDTH, OFFSET_Y+CLIP_HEIGHT);
  line( OFFSET_X, OFFSET_Y+CLIP_HEIGHT, OFFSET_X+CLIP_WIDTH, OFFSET_Y+CLIP_HEIGHT);
  fill(255, 0, 0); // 文字の色を赤に設定 (RGB)
  textSize(32); // 文字サイズを設定
  text(fname, 260, 444); // 文字列を表示
  fname = "";
}

//
// recover any error
//
void recover()
{
  serial.clear();
  serial.stop();
  delay(1000);
  while (true) {
    serial = new Serial(this, SERIAL_PORTNAME, SERIAL_BAUDRATE);
    if (serial != null) {
      break;
    }
    delay(1000);
  }
}

String timestamp()
{
  Date date = new Date();
  SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMdd-HHmmss-SSS");
  return sdf.format(date);
}
