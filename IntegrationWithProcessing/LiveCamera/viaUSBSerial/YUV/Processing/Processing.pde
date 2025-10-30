import java.util.Date;
import java.text.SimpleDateFormat;
import processing.serial.*;

Serial serial;

// Please change the serial setting for user environment
final String SERIAL_PORTNAME = "COM43";
final int    SERIAL_BAUDRATE = 921600;

void setup()
{
  // Please select the display size specified for a spresense sketch
  size(320, 240);   // QVGA
  //size(640, 480);   // VGA
  //size(1280, 960);  // QUADVGA
  //size(1280, 720);  // HD
  //size(1920, 1080); // FULLHD
  //size(2560, 1920); // 5M
  //size(2048, 1536); // 3M

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

void recieve_data()
{
  int size = 0;

  // Search sync words
  if (find_sync(3000)) {
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
  println(count++, ": size=", size, "time=", now - base_time, "[ms]");
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

  // Convert YUV422 to RGB and display
  int width = 320;
  int height = 240;
  PImage img = createImage(width, height, RGB);
  img.loadPixels();

  for (int i = 0, p = 0; i < data.length - 3 && p < img.pixels.length; i += 4) {
    int u  = data[i] & 0xFF;
    int y0 = data[i + 1] & 0xFF;
    int v  = data[i + 2] & 0xFF;
    int y1 = data[i + 3] & 0xFF;
    // convert YUV → RGB for two pixels
    img.pixels[p++] = yuvToRgb(y0, u, v);
    if (p < img.pixels.length)
      img.pixels[p++] = yuvToRgb(y1, u, v);
  }

  img.updatePixels();
  println("Converted YUV422 frame: " + img.width + "x" + img.height);
  image(img, 0, 0);

}

//
// YUV to RGB
//
color yuvToRgb(int y, int u, int v) {
  float r = y + 1.402 * (v - 128);
  float g = y - 0.344136 * (u - 128) - 0.714136 * (v - 128);
  float b = y + 1.772 * (u - 128);
  return color(constrain(r, 0, 255), constrain(g, 0, 255), constrain(b, 0, 255));
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
