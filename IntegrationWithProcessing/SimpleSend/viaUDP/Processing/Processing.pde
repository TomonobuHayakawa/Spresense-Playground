/*
 *  Draw Processing from Spresense
 *  Copyright 2023 Tomonobu Hayakawa
 *
 *  Orignal from https://github.com/baggio63446333/SpresenseCameraApps/tree/main/UsbLiveStreaming
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

import hypermedia.net.*;
import controlP5.*;

import java.util.*;

UDP udp;
ControlP5 cp5;

String msg = "test_messege";

final String IP = "192.168.2.109";
final int PORT = 10002;

void setup() {
  size(200, 200);

  udp = new UDP( this, 10001 );  
  udp.listen( true );

  textSize(24);

  UDP_Msg();

}

byte [] recieve_data;
boolean recieve_data_ready = false;

void UDP_Msg(){
  udp.send(msg,IP,PORT);
}

void receive( byte[] data, String ip, int port ) {

  if(!find_sync(data)){
    recover();
    return;
  }
  println( "receive: \""+data+"\" from "+ip+" on port "+port );
}


// for debug
boolean find_sync(byte[] data)
{
  String sync_words = "0000";

  for(int i=0;i<data.length;i++){
    println("recieve3 : " + sync_words);
    sync_words = sync_words.substring(1);
    sync_words = sync_words + (char)data[i];
    println("recieve5 : " + sync_words);
    if(sync_words.equals("SPRS")){
      recieve_data = Arrays.copyOfRange(data,i+1,data.length);
      recieve_data_ready = true;
      return true;
    }
  }
  return false;
}

void draw( )
{
  if(!recieve_data_ready) return;

  background(255);
  fill(0);

  
  int size = 0;

  size = recieve_data[3]<<24 | recieve_data[2]<<16 | recieve_data[1]<<8 | recieve_data[0];
  println("size = ", size);
  byte [] tmp = Arrays.copyOfRange(recieve_data,4,recieve_data.length);

  int i = 0;

  println("i=",i);
  if (tmp.length >= size) {
    println("data=",tmp[i]);
    text("data="+str(tmp[i]),10,20+i*30);
    i++;
    println("data=",tmp[i]);
    text("data="+str(tmp[i]),10,20+i*30);
    i++;

    float data_f = float(int(tmp[i] & 0xff) <<8 | (tmp[i+1] & 0xff))/100;
    println("tmp[i]=",(tmp[i] & 0xff));
    println("tmp[i+1]=",(tmp[i+1] & 0xff));
    println("tmp_i=",int(tmp[i] & 0xff) <<8 | (tmp[i+1] & 0xff));
    println("data_f=",data_f);
    text("data="+str(data_f),10,20+i*30);
  }

  recieve_data_ready = false;

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
