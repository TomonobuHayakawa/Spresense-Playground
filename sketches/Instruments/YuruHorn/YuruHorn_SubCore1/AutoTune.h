#ifndef _AUTO_TUNE_H_
#define _AUTO_TUNE_H_

class AutoTuneClass
{
public:
  void begin();
  void set();
  float get(float);
  void end(){}

private:

  struct NoteStruct {
    int note_number;
    char note_name[4];
    float note_fq;
  };

  struct NoteStruct note[128];

  const int concert_pitch = 440;

  struct OctaveStruct {
    int note_number;
    char note_name[4];
  };

  const struct OctaveStruct octave[12] = {
    {0, "C"},
    {1, "C#"},
    {2, "D"},
    {3, "D#"},
    {4, "E"},
    {5, "F"},
    {6, "F#"},
    {7, "G"},
    {8, "G#"},
    {9, "A"},
    {10, "A#"},
    {11, "B"}
  };

//  enum CodeType {
  
//  struct CodeStruct 
//  int key_value = 4;  // Key = E
//  const char *scale_name = "Maj"; 
//  int ommit_note[5] = { 1, 3, 6, 8, 10 };  // Major Scale

  int key_value = 0;  // Key = C
  const char *scale_name = "Oki"; 
  int ommit_note[6] = { 1, 3, 5, 6, 8, 10 };  // Okinawa Scale

  int get_ommit_size(){ return sizeof ommit_note / sizeof(int); } // int -> int8_t

  int range_number;
  int octave_shift = 0;

  struct RangeStruct {
    int note_number;
    char note_name[4];
    float lower;
    float just;
    float upper;
  };

  struct RangeStruct range[113];

};

extern AutoTuneClass AutoTune;

#endif
