#include <MIDI.h>
#include <Audio.h>

AudioClass *theAudio;

MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI);

class Note2Fs {
  typedef struct {
    uint8_t pitch;  
    int     fs;
  } Key;

private:
  const Key table[12] =
  {
    {33,55},
    {34,58},
    {35,62},
    {36,65},
    {37,69},
    {38,73},
    {39,78},
    {40,82},
    {41,87},
    {42,93},
    {43,98},
    {44,104}
  };

public:
  int getFs(uint8_t pitch){
    return table[(pitch-33)%12].fs*pow(2,((pitch-33)/12));
  }
};

Note2Fs theNote2Fs;

static int8_t  key_count = 0;
static int32_t volume    = -12;

// -----------------------------------------------------------------------------

// This function will be automatically called when a NoteOn is received.
// It must be a void-returning function with the correct parameters,
// see documentation here:
// http://arduinomidilib.fortyseveneffects.com/a00022.html

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
    // Do whatever you want when a note is pressed.

    // Try to keep your callbacks short (no delays ect)
    // otherwise it would slow down the loop() and have a bad impact
    // on real-time performance.

    printf("pitch=%d\n",pitch);
    printf("cont=%d\n",key_count);
    printf("fs=%d\n",theNote2Fs.getFs(pitch));
    
//    for(int i=0;i<sizeof(table);i++){
//    printf("pitch i =%d\t",table[i].pitch);
//    printf("fs i =%d\n",table[i].fs);
/*      if(table[i].pitch == pitch){
        key_count++;
        theAudio->setBeep(1,-40,table[i].fs);
      }*/
      key_count++;
      theAudio->setBeep(1,volume,theNote2Fs.getFs(pitch));
      
//    }
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
    // Do something when the note is released.
    // Note that NoteOn messages with 0 velocity are interpreted as NoteOffs.

    key_count--;
    printf("cont down=%d\n",key_count);
    if(key_count <= 0){
      theAudio->setBeep(0,volume,262);
    }
}

#define CONTROL_CHANGE_VOLUME  7
#define CONTROL_CHANGE_PAN    10

void handleControlChange(byte channel , byte number , byte value)
{
    printf("channel=%d\n",channel);
    printf("number=%d\n",number);
    printf("value=%d\n",value);
    switch(number){
     case CONTROL_CHANGE_VOLUME:
          volume = -12-3*(int32_t)sqrt(value);
          printf("volume=%d\n",volume);
          break;
     case CONTROL_CHANGE_PAN:
          break;
     default:
          break;
    }

}

// -----------------------------------------------------------------------------

void setup()
{
  theAudio = AudioClass::getInstance();
  theAudio->begin();
  puts("initialization Audio Library");

  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP);

    // Connect the handleNoteOn function to the library,
    // so it is called upon reception of a NoteOn.
    MIDI.setHandleNoteOn(handleNoteOn);  // Put only the name of the function

    // Do the same for NoteOffs
    MIDI.setHandleNoteOff(handleNoteOff);

    MIDI.setHandleControlChange(handleControlChange);

    // Initiate MIDI communications, listen to all channels
    MIDI.begin(MIDI_CHANNEL_OMNI);

    key_count = 0;
}

void loop()
{
    // Call MIDI.read the fastest you can for real-time performance.
    MIDI.read();

    // There is no need to check if there are messages incoming
    // if they are bound to a Callback function.
    // The attached method will be called automatically
    // when the corresponding message has been received.
}
