#include <Wire.h>
#define lcd_address 0x3e // LCD slave

#if (SUBCORE != 2)
#error "Core selection is wrong!!"
#endif

#include <MP.h>

#include <math.h>


int i2cwritecmd(byte cmd) {
    Wire.beginTransmission(lcd_address);
    Wire.write((byte)0x00); 
    Wire.write(cmd);
    return Wire.endTransmission();
}
 
int i2cwritedata(byte data) {
    Wire.beginTransmission(lcd_address);
    Wire.write(0x40);
    Wire.write(data);
    return Wire.endTransmission();
}
 
void lcdcu_set(int x, int y) {
    byte ca = (x + y * 0x40) | (0x80); i2cwritecmd(ca);
}
 
void lcdclear() {
    i2cwritecmd(0x01);
}
 
void lcdhome() {
    i2cwritecmd(0x02);
}
 
void dsift_l() {
 i2cwritecmd(0x1C);
}
 
void dsift_r() {
    i2cwritecmd(0x18);
}
 
void i2cprint( String pdata) {
    int n = pdata.length();
    for (int i = 0; i < n; i = i + 1) {
        i2cwritedata(pdata.charAt(i));
        delay(1);
    }
}
 
void init_lcd() {
    delay(145);
    i2cwritecmd(0x38);delay(1);
    i2cwritecmd(0x39);delay(1);
    i2cwritecmd(0x14);delay(1);
    i2cwritecmd(0x71);delay(1);
    i2cwritecmd(0x56); //3.3V
    delay(1);
    i2cwritecmd(0x6c);delay(300);
    i2cwritecmd(0x38);delay(1);
    i2cwritecmd(0x01);delay(2);
    i2cwritecmd(0x0C);delay(2);
}


int concert_pitch = 440;

struct OctaveStruct {
  int note_number;
  char note_name[4];
};

struct OctaveStruct octave[12] = {
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

struct NoteStruct {
  int note_number;
  char note_name[4];
  float note_fq;
};

struct NoteStruct note[128];

// int key_value = 4;  // Key = E
// const char *scale_name = "Maj"; 
// int ommit_note[5] = { 1, 3, 6, 8, 10 };  // Major Scale

 int key_value = 0;  // Key = C
 const char *scale_name = "Oki"; 
 int ommit_note[6] = { 1, 3, 5, 6, 8, 10 };  // Okinawa Scale

int ommit_size = sizeof ommit_note / sizeof(int);
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

void setup()
{
  Serial.begin(115200);
  MP.begin();
  MP.RecvTimeout(MP_RECV_POLLING);

  Wire.begin();
  init_lcd();

  /* ----- Display Setting ----- */
  lcdhome();
  i2cprint(String(octave[key_value].note_name));
  i2cprint(String(scale_name));
  lcdcu_set( 5, 0 );
  i2cprint(String(concert_pitch));

  printf("Pitch = %d\n", concert_pitch);
  printf("Key = %d %s + shift %d\n", key_value, octave[key_value].note_name, octave_shift);
  for (int o = 0; o < ommit_size; o++) {
    int ommit_shift = (ommit_note[o] + key_value) % 12;
    printf("%d/%s, ", ommit_shift, octave[ommit_shift].note_name);
  }
  printf("ommit %d notes\n", ommit_size);

  /* ----- Set C0 to G9 ----- */
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
  }

  /* ----- Select Use Note ----- */
  int range_setter = 0;
  for (int i = 0; i < 113; i++) {
    int j = i + 13;
    int ok = 0;
    for (int o = 0; o < ommit_size; o++) {
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

#define PIN 17

void loop()
{
  int      ret;
  int8_t   msgid;
  int      *peak_fq;

  ret = MP.Recv(&msgid, &peak_fq);
  if (ret >= 0) {

    int peakFsi = (int) peak_fq;
    float peakFs = (float) peakFsi / 100;
  
    for (int i = 1; i < range_number; i++) {
      if (range[i].lower <= peakFs && peakFs < range[i].upper) {
        int tone_fq = (int) range[i + octave_shift].just;
        tone(PIN, tone_fq, 150);

//        printf( " %s , %d\n", range[i].note_name, tone_fq );

        lcdcu_set( 0, 1 );
        i2cprint(String(range[i].note_name));
        int s = i % 12;
        if (s == 0 || s == 2 || s == 4 || s == 5 || s == 7 || s == 9 || s == 11) {
          lcdcu_set( 2, 1 );
          i2cprint(String(" "));
        }
        lcdcu_set( 5, 1 );
        i2cprint(String(tone_fq));

        break;
      }
    }
  } 
}
