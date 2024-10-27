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

// for debug
int count = 0;
int base_time = 0;

int sx = 0;
int sy = 0;
int width = 0;
int height = 0;

void recieve_data()
{
  int size = 0;

  // Search sync words
  if (find_sync(3000)) {
    // Receive a result in 2byte
  	sx = (serial.read()<<8 | serial.read())*2;
    println("sx =", sx);
  	sy = (serial.read()<<8 | serial.read())*2;
    println("sy =", sy);
  	width = (serial.read()<<8 | serial.read())*2;
    println("width =", width);
  	height = (serial.read()<<8 | serial.read())*2;
    println("height =", height);

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

  // Save data as a JPEG file and display it
  saveBytes(dataPath("sample.jpg"), data);
  PImage img = loadImage(dataPath("sample.jpg"));
//  println("x = ",img.width,"  y = ",img.height);
  image(img, 0, 0);

  stroke( #ff0000 );
  strokeWeight( 2 );
  if((width != 0)|| (width != 0)){
    line( sx, sy, sx+width, sy);
    line( sx, sy, sx, sy+height);
    line( sx+width, sy, sx+width, sy+height);
    line( sx, sy+height, sx+width, sy+height);
  }
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
