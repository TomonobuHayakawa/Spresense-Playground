#ifndef KEY_INFO_H_
#define KEY_INFO_H_

#include <math.h>
#include <string.h>
#include <stdio.h>

class KeyInfo
{
public:
  void  begin(){ clear();}
  void  end(){}

  void clear(){
    memset(cur,0,number_of_keyinfo);
    memset(past,0,number_of_keyinfo);
  }

  bool set(int pos, int bits){
    cur[pos] = cur[pos] | (0x01 << bits);
  }
  
  unsigned char* adr(){
    return cur;
  }

  void update(){
    for(int i=0;i<number_of_keyinfo;i++) {
      past[i] = cur[i];
    }
  }
  
  void print(){
    for(int i=0;i<number_of_keyinfo;i++) {
      printf("%x ",cur[i]);
    }
    putchar('\n');
  }
  bool get(int pos, int bits){
    return cur[pos] & (0x01 << bits);
  }
  
  bool is_change(int pos, int bits){
    return (cur[pos] & (0x01 << bits)) != (past[pos] & (0x01 << bits));
  }

  static const unsigned int max_bits(){return number_of_keybits;}

  static const unsigned int max_pos(){return number_of_keyinfo;}

private:

  static const unsigned int number_of_keyinfo = 7;
  static const unsigned int number_of_keybits = 5;

  unsigned char cur[number_of_keyinfo];
  unsigned char past[number_of_keyinfo];

};

#endif /* KEY_INFO_H_ */
