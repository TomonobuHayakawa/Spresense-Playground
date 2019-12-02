#include "AutoTune.h"

#include <math.h>
#include <string.h>
#include <stdio.h>


void AutoTuneClass::begin()
{
  for (int i = 12; i < 128; i++) {
    int pow_i = i - 69;
    double pow_note_number = (double) pow_i;
    double pow_fq = pow_note_number / (double) 12;
    double note_fq_double = (double) concert_pitch * pow( (double) 2, pow_fq);
    float note_fq = (float) note_fq_double;

    note[i].note_number = i;

    char *pow_note_name = octave[i % 12].note_name;
    
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
    
    strcpy(note[i].note_name, pow_name);
    note[i].note_fq =  note_fq;
//    printf("%d,%s,%8.3f\n",note[i].note_number,note[i].note_name,note[i].note_fq);
  }
}


void AutoTuneClass::set()
{
  /* ----- Select Use Note ----- */
  int range_setter = 0;
  for (int i = 0; i < 113; i++) {
    int j = i + 13;
    int ok = 0;
    for (int o = 0; o < get_ommit_size(); o++) {
      int ommit_shift = (ommit_note[o] + key_value) % 12;
      if ( j % 12 == ommit_shift ) {
//        printf("ommit %d , %s\n", ommit_shift, octave[ommit_shift].note_name);
        ok = -1;
        break;
      }
    }
    if (ok == 0) {
      range[range_setter].note_number = note[j].note_number;
      strcpy(range[range_setter].note_name, note[j].note_name);
      range[range_setter].just = (note[j].note_fq);
      range_setter++;      
    }
  }
  range_number = range_setter - 1;

  /* ----- Set Range ----- */
  for (int i = 1; i < range_number; i++) {
    range[i].lower = ( range[i-1].just + range[i].just ) / 2;
    range[i].upper = ( range[i].just + range[i+1].just ) / 2;
  }

//  for (int i = 1; i < range_number; i++) {
//    printf( "%d %d %s L%4.2f J%4.2f U%4.2f\n", i, range[i].note_number, range[i].note_name, range[i].lower, range[i].just, range[i].upper);
//  }

}

float AutoTuneClass::get(float peakFs)
{
    for (int i = 1; i < range_number; i++) {
      if (range[i].lower <= peakFs && peakFs < range[i].upper) {
        return range[i + octave_shift].just;
      }
    }
}

AutoTuneClass AutoTune;
