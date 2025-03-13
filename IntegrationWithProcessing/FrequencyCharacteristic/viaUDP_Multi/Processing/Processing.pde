import hypermedia.net.*;
import controlP5.*;

import java.util.*;
import java.io.*;

UDP udp;
ControlP5 cp5;

// Please change the serial setting for user environment
final String IP = "192.168.2.20";
final int PORT = 10002;

//final String DATA_TYPE = "fft";
final String DATA_TYPE = "raw";

//String MODE_TYPE = "draw";
String MODE_TYPE = "file";

final String  SAVE_FILE0_NAME = "data/pcm0.raw";
final String  SAVE_FILE1_NAME = "data/pcm1.raw";
int           SAVE_DATA_SIZE = 1000;

static int frame_sample = 1024;
static int max_data_number = frame_sample;

String msg = "test_messege";
OutputStream output0;
OutputStream output1;

void setup()
{
  size(800, 800);
  background(255);

  udp = new UDP( this, 10001 );  
  udp.listen( true );

  if(DATA_TYPE.equals("raw")){
    if(MODE_TYPE.equals("file")){
        output0 = createOutput(SAVE_FILE0_NAME);
        output1 = createOutput(SAVE_FILE1_NAME);
    }
  }

  UDP_Msg();
}

void UDP_Msg(){
  udp.send(msg,IP,PORT);
}

byte [] recieve_data;
boolean recieve_data_ready = false;
boolean draw_ready = true;
int channel_number = 0;
int recieve_number = 0;
int rest_number = 0;
int frame_no = 0;

void receive( byte[] data, String ip, int port ) {
   
  if(rest_number > 0){
    byte [] tmp = concat(recieve_data,data);
    recieve_data = tmp;
    rest_number -= (data.length);
  }else if(!find_sync(data)){
    recover();
    return;
  }
  if(rest_number <= 0){
    recieve_data_ready = true;

    int now = millis();
//    println( "receive: \""+data+"\" from "+ip+" on port "+port , "time=", now - base_time, "[ms]");
    base_time = now;
  }

  if(MODE_TYPE.equals("file")){
    if(DATA_TYPE.equals("raw")){
      save_data(recieve_data,recieve_data.length);
    }else{
      save_fft_data(recieve_data,recieve_data.length);
    }
  }

}

boolean find_sync(byte[] data)
{
  String sync_words = "0000";
  for(int i=0;i<data.length;){
    sync_words = sync_words.substring(1);
    sync_words = sync_words + (char)data[i];
    i++;
    if(sync_words.equals("SPRS")){
      if(draw_ready == true){
      channel_number = data[i+1] & 0xff;
      channel_number <<= 8;
      channel_number += data[i] & 0xff;
      i += 2;
      
      recieve_number = data[i+1] & 0xff;
      recieve_number <<= 8;
      recieve_number += (data[i] & 0xff) + 1;
      recieve_number *= 2;
      i += 2;

      frame_no = data[i+3] & 0xff;
      frame_no <<= 8;
      frame_no += data[i+2] & 0xff;
      frame_no <<= 8;
      frame_no += data[i+1] & 0xff;
      frame_no <<= 8;
      frame_no += data[i] & 0xff;
      i += 4;
//      println("recieve_number = ",recieve_number);
//      println("data.length = ",data.length);
      println("frame_no = ",frame_no);
      println("channel_number = ",channel_number);
        recieve_data = Arrays.copyOfRange(data,(i),data.length);
        rest_number = recieve_number - (data.length) + i;
        if(rest_number <= 0) recieve_data_ready = true;
      }else{
        println("drop frame");
      }
      return true;
    }
  }
  return false;
}

void draw()
{
  if(!recieve_data_ready) return;

  if(MODE_TYPE.equals("draw")){
     draw_graph(recieve_data,recieve_data.length);
     recieve_data_ready = false;
  }
}

// for debug
int count = 0;
int base_time = 0;

void save_data(byte[]data, int size)
{
//  println("draw size = "+(size-2));
  
  for (int i=0; i < size-2; i=i+2) {
    try {
      if(SAVE_DATA_SIZE<0){
        output0.flush();
        output1.flush();
        output0.close();
        output1.close();
        exit();
      }
      if(channel_number == 0){
        output0.write(byte(data[i+1] & 0xff));
        output0.write(byte(data[i] & 0xff));
      }else{
        output1.write(byte(data[i+1] & 0xff));
        output1.write(byte(data[i] & 0xff));
      }
    } catch (IOException e) {
      e.printStackTrace();
    }
  }
  SAVE_DATA_SIZE--;
}

void save_fft_data(byte[]data, int size)
{
  println("draw size = "+(size-4));
  
  String  FFT_FILE_NAME = "data/fft.dat";
  DataOutputStream dout = new DataOutputStream(createOutput(FFT_FILE_NAME));
  for (int i=0; i < size-2; i=i+2) {
    try {
      if(SAVE_DATA_SIZE<0){
        dout.flush();
        dout.close();
        exit();
      }
      float data_f = float(int(data[i+1] & 0xff) <<8 | (data[i] & 0xff))/1000;
      dout.writeFloat(data_f);
      println(data_f);
    } catch (IOException e) {
      e.printStackTrace();
    }
  }
  SAVE_DATA_SIZE--;
    try {
        dout.flush();
        dout.close();
        exit();
    } catch (IOException e) {
      e.printStackTrace();
    }
    MODE_TYPE = "draw";
}

void draw_graph(byte[]data, int size)
{
   background(255);

   if(size > 550) size = 520; //表示の速度のため制限してる

   draw_ready = false;
   for (int i=4; i < size-4; i=i+2) {

if(DATA_TYPE.equals("fft")){
    float data_f_s = float(int(data[i+1] & 0xff) <<8 | (data[i] & 0xff))/1000;
    float data_f_e = float(int(data[i+3] & 0xff) <<8 | (data[i+2] & 0xff))/1000;
    float stx = map(i/2, 0,size/2, 0, width);
    float sty = map(data_f_s, 0, 50, height, 0);
    float etx = map(i/2+1, 0, size/2, 0, width);
    float ety = map(data_f_e, 0,50, height, 0);
    line(stx, sty, etx, ety);
}else{
  if(channel_number == 0){
    int data_i_s = int(data[i+1] << 8 | (data[i] & 0xff));
    int data_i_e = int(data[i+3] << 8 | (data[i+2] & 0xff));
    int stx = (int)map(i, 0,size, 0, width);
    int sty = (int)map(data_i_s, -32768, 32767, height/2, 0);
    int etx = (int)map(i, 0, size, 0, width);
    int ety = (int)map(data_i_e, -32768, 32767, height/2, 0);
    line(stx, sty, etx, ety);
  }else{
    int data_i_s = int(data[i+1] << 8 | (data[i] & 0xff));
    int data_i_e = int(data[i+3] << 8 | (data[i+2] & 0xff));
    int stx = (int)map(i, 0,size, 0, width);
    int sty = (int)map(data_i_s, -32768, 32767, height/2, 0);
    int etx = (int)map(i, 0, size, 0, width);
    int ety = (int)map(data_i_e, -32768, 32767, height/2, 0);
    line(stx, sty+height/2, etx, ety+height/2);
  }
}
   } 
   
  draw_ready = true;

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
