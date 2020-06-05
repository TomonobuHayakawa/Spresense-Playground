#include "CodeController.h"

void CodeController::begin()
{
  for (int i = 12; i < 128; i++) {
    int pow_i = i - 69;
    double pow_note_number = (double) pow_i;
    double pow_fq = pow_note_number / (double) 12;
    double note_fq_double = (double) concert_pitch * pow( (double) 2, pow_fq);
    float  note_fq = (float) note_fq_double;

    note[i].number = i;

    char *pow_note_name = octave[i % 12].name;
    
    int pow_octave = i / 12 - 1;
    char pow_octave_number = '0' + pow_octave;

    char pow_name[4];
    const char check_sharp[2] = {'#', '\0'};
    if (pow_note_name[1] != check_sharp[0]) {
      pow_name[0] = pow_note_name[0];
      pow_name[1] = pow_octave_number;
      pow_name[2] = '\0'; 
    } else {
      pow_name[0] = pow_note_name[0];
      pow_name[1] = pow_note_name[1];
      pow_name[2] = pow_octave_number;
      pow_name[3] = '\0'; 
    };
    
    strcpy(note[i].name, pow_name);
    note[i].fq =  note_fq;
//    printf("%d,%s,%8.3f\n",note[i].number,note[i].name,note[i].fq);
  }

}

int CodeController::get(const char* str)
{
  for (int i = 12; i < 128; i++) {
  	if(!(note[i] == str)) {
      return round(note[i].fq);
    }
  }
  return 0;
}

int CodeController::get(const char* str, int no)
{
  for (int i = 0; i < sizeof(codes); i++) {
   if(strcmp(str,codes[i].name)==0) {
   	return get(codes[i].get(no));
    }
  }
  return 0;
}
