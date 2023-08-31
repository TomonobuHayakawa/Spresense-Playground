import hypermedia.net.*;
import controlP5.*;

import java.util.*;

UDP udp;
ControlP5 cp5;

// Please change the serial setting for user environment
final String IP = "192.168.2.109";
final int PORT = 10002;

final boolean VIEW_RAW_DATA = true;
//final boolean VIEW_RAW_DATA = false;

//static int frame_sample = 1024;
//static int data_number = frame_sample / 2;
static int data_number = (700+1);

String msg = "test_messege";

void setup()
{
  size(800, 400);
  background(255);

  udp = new UDP( this, 10001 );  
  udp.listen( true );

  UDP_Msg();
}

void UDP_Msg(){
  udp.send(msg,IP,PORT);
}

void receive( byte[] data, String ip, int port ) {

  if(!find_sync(data)){
    recover();
    return;
  }
  int now = millis();
  println( "receive: \""+data+"\" from "+ip+" on port "+port , "time=", now - base_time, "[ms]");
  base_time = now;
}

int pos=0;

float [] data_f = new float[data_number];
short [] data_i = new short[data_number];

byte [] recieve_data;
boolean recieve_data_ready = false;

boolean find_sync(byte[] data)
{
  String sync_words = "0000";

  for(int i=0;i<data.length;i++){
    sync_words = sync_words.substring(1);
    sync_words = sync_words + (char)data[i];
    if(sync_words.equals("SPRS")){
      recieve_data = Arrays.copyOfRange(data,(i+1),(data.length));
      recieve_data_ready = true;
      return true;
    }
  }
  return false;
}

void draw()
{
  if(!recieve_data_ready) return;
  
   draw_graph(recieve_data,recieve_data.length);
   
   recieve_data_ready = false;

}

// for debug
int count = 0;
int base_time = 0;

void draw_graph(byte[]data, int size)
{
   background(255);

   for (int i=4; i < size-4; i=i+2) {

if(!VIEW_RAW_DATA){
    float data_f_s = float(int(data[i+1] & 0xff) <<8 | (data[i] & 0xff))/1000;
    float data_f_e = float(int(data[i+3] & 0xff) <<8 | (data[i+2] & 0xff))/1000;
    float stx = map(i/2, 0,size/2, 0, width);
    float sty = map(data_f_s, 0, 50, height, 0);
    float etx = map(i/2+1, 0, size/2, 0, width);
    float ety = map(data_f_e, 0,50, height, 0);
    line(stx, sty, etx, ety);
}else{
    int data_i_s = int(data[i+1] << 8 | (data[i] & 0xff));
    int data_i_e = int(data[i+3] << 8 | (data[i+2] & 0xff));
    int stx = (int)map(i, 0,size, 0, width);
    int sty = (int)map(data_i_s, -32768, 32767, height, 0);
    int etx = (int)map(i, 0, size, 0, width);
    int ety = (int)map(data_i_e, -32768, 32767, height, 0);
    line(stx, sty, etx, ety);
}

   } 
}

//
// recover any error
//
void recover()
{
  udp.listen( false );
  delay(1000);
  udp.listen( true );
  delay(1000);
}
