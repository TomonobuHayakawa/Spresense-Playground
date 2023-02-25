import java.util.Date;
import java.text.SimpleDateFormat;
import processing.serial.*;

Serial serial;

// Please change the serial setting for user environment
final String SERIAL_PORTNAME = "COM10";
final int    SERIAL_BAUDRATE = 921600;

static int frame_sample = 1024;
//static int data_number = frame_sample / 2;
static int data_number = 400;

void setup()
{
  size(800, 300);
  background(255);

  serial = new Serial(this, SERIAL_PORTNAME, SERIAL_BAUDRATE);
}

int pos=0;

//byte [] data = new byte[data_number];
float [] data = new float[data_number];

void draw()
{
   recieve_data();
   draw_graph();
   pos++;
}

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
  
// for debug
int count = 0;
int base_time = 0;

void draw_graph()
{
   background(255);
   for (int i=1; i< data_number;i++) {
     float stx = map(i-1, 0,data_number, 0, width);
     float sty = map(data[i-1], 0, 100, height, 0);
     float etx = map(i, 0, data_number, 0, width);
     float ety = map(data[i], 0,100, height, 0);
     line(stx, sty, etx, ety);
   }
  
}
  
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
  int timeout = millis() + 5000;
  for (int i = 0; i < size/2; ) {
    if (serial.available() > 0) {
      data[i] = (float)((serial.read() | serial.read()<<8)/100);
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
