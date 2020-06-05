#ifndef CODE_CONTROLLER_H_
#define CODE_CONTROLLER_H_

#include <math.h>
#include <string.h>
#include <stdio.h>

class CodeController
{
public:
  void  begin();
  void  end(){}

  int   get(const char*);
  int   get(const char*,int);

private:

  struct NoteStruct {
    int   number;
    char  name[4];
    float fq;
    int operator == (const char* str) {
      return strcmp(str,name);
    }
  };

  struct NoteStruct note[128];

  const int concert_pitch = 440;

  struct OctaveStruct {
    int  number;
    char name[4];
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

  struct CodeStruct {
    char name[5];
    char note[4][4];
    char* get(int no) { return note[no][4]; }
  };
  
  const struct CodeStruct codes[3] = {
    {"C",  {"C3", "E4", "G4" ,"C5" }},
    {"C#m",{"C#3","E4", "G#4","C#5"}},
    {"D",  {"D3", "F#4","A4", "D5" }},
/*        'D': ['D3','F#4','A4','D5'],
        'D/C': ['C3','D4','F#4','A4'],
        'D/F#': ['F#3','F#4','A4','D5'],
        'E': ['E3','G#4','B4','E5'],
        'E7': ['E3','G#4','B4','D5'],
        'Em': ['E3','G4','B4','E5'],
        'F#7': ['A#3','C#4','E4','F#4'],
        'G': ['G3','B4','D5','G5'],
        'G#7': ['C3','D#4','F#4','G#4'],
        'A': ['A3','C#4','E4','A4'],
        'B': ['B3','D#4','F#4','B4'],
        'B/D#': ['D#3','D#4','F#4','B4'],
        'B7': ['B3','D#4','F#4','A4'],
        'Bm7': ['B3','D4','F#4','A4']*/
  };
};

#endif /* CODE_CONTROLLER_H_ */
