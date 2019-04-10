/*
 *  config.h - WiFi Configration Header
 *
 *  This work is free software; you can redistribute it and/or modify it under the terms 
 *  of the GNU Lesser General Public License as published by the Free Software Foundation; 
 *  either version 2.1 of the License, or (at your option) any later version.
 *
 *  This work is distributed in the hope that it will be useful, but without any warranty; 
 *  without even the implied warranty of merchantability or fitness for a particular 
 *  purpose. See the GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License along with 
 *  this work; if not, write to the Free Software Foundation, 
 *  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

/*-------------------------------------------------------------------------*
 * AP Configration
 *-------------------------------------------------------------------------*/
#define  AP_SSID     "xxxxxxxxxx"
#define  PASSPHRASE  "xxxxxxxxxx"

/*-------------------------------------------------------------------------*
 * Radio Configration
 *-------------------------------------------------------------------------*/
#define ListNum 6

class RadioList
{
public:
  next(){ index = (index+1) % ListNum; }
  prev(){ index = (ListNum+index-1)%ListNum; }
  char* name(){ return list[index].name; }
  char* ip(){ return list[index].ip; }
  char* port(){ return list[index].port; }
  char* site(){
    String str("GET /");
    str = link(str,list[index].file);
    str = link(str," HTTP/1.1\r\nHOST: ");
    str = link(str,list[index].ip);
    str = link(str,":");
    str = link(str,list[index].port);
    str = link(str,"\r\nConnection: close\r\n\r\n");
    return str.c_str();
  }

private:

  String link(String s,char* p){
    String str(p);
    return (s+str);
  }

  int index = 1;

  typedef struct s_RadioInfo {
    char* name;
    char* file;
    char* ip;
    char* port;  
  }RadioInfo;

  RadioInfo list[ListNum] =
  {
    {"J-Pop Sakura","stream","158.69.38.195","20278"},
    {"Dance Wave","dance.mp3","dancewave.online","8080"},
    {"Dance Wave Retro","retrodance.mp3","dancewave.online","8080"},
    {"Mountain Reggae Radio","radio80and90hits","5.135.154.72","12570"},
    {"xxxRock.fm","stream","23.29.71.154","8140"},
    {"The Disco Palace","stream","144.217.129.213","8396"}
  };
};

#endif /*_CONFIG_H_*/
