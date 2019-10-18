/*
Data flow diagram
+---------------+     +---------------+     +---------------+     +---------------+     +---------------+     +---------------+     
|instrument1 rom|     |instrument2 rom|     |instrument3 rom|     |instrument4 rom|     |instrument5 rom|     |instrument6 rom|
+---------------+     +---------------+     +---------------+     +---------------+     +---------------+     +---------------+
        |                     |                     |                     |                     |                     |
        +---------------------+---------------------+---------------------+---------------------+---------------------+
                                    |
                                    V
                           +--------------------+            +-------------------+          +---------------------+
                           |instrXPatternPointer|            |instrXsamplePointer| <--------|rpi available sample |
                           +--------------------+            +-------------------+          +---------------------+
                                       |                                |
                                       +--------------------------------+
                                                      |
                                                      V
                                             actual playing instr.
                     morphSample  <------>   drumKitSamplePlaying   <------> encoderSampleChange
                     morphPattern <------>   drumKitPatternPlaying  <------> encoderPatternChange
                                                      |
                                                 ramPatterns
*/

#define DO_SERIAL false
#define DO_MIDIUSB false
#define DO_MIDIHW true

#ifdef DO_MIDIHW
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();
#endif

#ifdef DO_MIDIUSB
#include "MIDIUSB.h"
#endif

#include <PantalaDefines.h>
#include <EventDebounce.h>
#include <Counter.h>
//#include <Trigger.h>
#include <Rotary.h>
#include <DueTimer.h>

#define I2C_ADDRESS 0x3C
#define DISPLAY_WIDTH 128 // OLED Oled width, in pixels
#define DISPLAY_HEIGHT 64 // OLED Oled height, in pixels
#define TEXTLINE_HEIGHT 9 // OLED Oled height, in pixels

//adafruit
#include <Adafruit_SSD1306.h>
#define OLED_RESET 47 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, OLED_RESET);

#define OLEDUPDATETIME 35000 //microsseconds to update display
#define MAXSTEPS 64          //max step sequence
#define MYCHANNEL 1          //midi channel for data transmission
#define MAXENCODERS 8        //total of encoders
#define MOODENCODER 0        //total of sample encoders
#define MORPHENCODER 1       //total of sample encoders
#define MAXINSTRUMENTS 6     //total of sample encoders
#define MIDICHANNEL 1        //standart drum midi channel
#define MOODMIDINOTE 20      //midi note to mood message
#define MORPHMIDINOTE 21     //midi note to morph message
#define KICKCHANNEL 0
#define SNAKERCHANNEL 1 //snare + shaker
#define CLAPCHANNEL 2
#define HATCHANNEL 3
#define OHHRIDECHANNEL 4 //open hi hat + ride
#define PERCCHANNEL 5
#define MAXINSTR1SAMPLES 3
#define MAXINSTR2SAMPLES 4
#define MAXINSTR3SAMPLES 5
#define MAXINSTR4SAMPLES 5
#define MAXINSTR5SAMPLES 5
#define MAXINSTR6SAMPLES 3
#define MAXINSTR1PATTERNS 3
#define MAXINSTR2PATTERNS 5
#define MAXINSTR3PATTERNS 5
#define MAXINSTR4PATTERNS 5
#define MAXINSTR5PATTERNS 5
#define MAXINSTR6PATTERNS 4
#define DOTGRIDINIT 36
#define GRIDPATTERNHEIGHT 4
#define BKPPATTERN 0

//PINS
#define SDAPIN 20
#define SCLPIN 21
#define MIDITXPIN 18
#define ENCBUTPINMOOD 11
#define ENCBUTPINMORPH 8
#define ENCBUTPININSTR1 40
#define ENCBUTPININSTR2 34
#define ENCBUTPININSTR3 46
#define ENCBUTPININSTR4 22
#define ENCBUTPININSTR5 52
#define ENCBUTPININSTR6 28
#define TRIGOUTPIN1 5
#define TRIGOUTPIN2 3
#define TRIGOUTPIN3 16
#define TRIGOUTPIN4 29
#define TRIGOUTPIN5 23
#define TRIGOUTPIN6 27
#define ENCPINAMOOD 13
#define ENCPINBMOOD 12
#define ENCPINAMORPH 10
#define ENCPINBMORPH 9
#define ENCPINAINSTR1 38
#define ENCPINBINSTR1 36
#define ENCPINAINSTR2 32
#define ENCPINBINSTR2 30
#define ENCPINAINSTR3 44
#define ENCPINBINSTR3 42
#define ENCPINAINSTR4 14
#define ENCPINBINSTR4 15
#define ENCPINAINSTR5 50
#define ENCPINBINSTR5 48
#define ENCPINAINSTR6 26
#define ENCPINBINSTR6 24
#define ACTIONPININSTR1 6
#define ACTIONPININSTR2 4
#define ACTIONPININSTR3 2
#define ACTIONPININSTR4 17
#define ACTIONPININSTR5 31
#define ACTIONPININSTR6 25
#define ENCBUTMOOD 0
#define ENCBUTMORPH 1
#define ENCBUTINSTR1 2
#define ENCBUTINSTR2 3
#define ENCBUTINSTR3 4
#define ENCBUTINSTR4 5
#define ENCBUTINSTR5 6
#define ENCBUTINSTR6 7

volatile uint32_t u_lastTick;               //last time tick was called
volatile uint32_t u_tickInterval = 1000000; //tick interval
boolean updateOledUpdateTimeShift = false;  //flag to update time shift
int32_t u_timeShift = 1100;                 //default time shift keep the 100 microsseconds there
int32_t u_timeShiftStep = 1000;             //time shift amount update step
#define u_timeShiftLimit 20000              //time shift + and - limit
volatile uint32_t sampleChangeWindowEndTime;
boolean midiArray[6];
uint8_t playMidiValue = 0; //calculate final noteVelocity

uint16_t bpm = 125;
uint32_t u_bpm = 0;
EventDebounce interfaceEvent(200); //min time in ms to accept another interface event

uint8_t instrSampleMidiNote[MAXINSTRUMENTS] = {10, 11, 12, 13, 14, 15};  //midi note to sample message
uint8_t instrPatternMidiNote[MAXINSTRUMENTS] = {20, 21, 22, 23, 24, 25}; //midi note to pattern message

#define MAXGATELENGHTS 8
#define DEFAULTGATELENGHT 5000
#define EXTENDEDGATELENGHT 40000
int8_t gateLenghtSize[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
volatile int8_t gateLenghtCounter = 0;

// Trigger trigger1(TRIGOUTPIN1);
// Trigger trigger2(TRIGOUTPIN2);

uint8_t triggerPins[MAXINSTRUMENTS] = {TRIGOUTPIN1, TRIGOUTPIN2, TRIGOUTPIN3, TRIGOUTPIN4, TRIGOUTPIN5, TRIGOUTPIN6};
//encoders buttons : mood, morph, instruments
uint8_t encoderButtonPins[MAXENCODERS] = {ENCBUTPINMOOD, ENCBUTPINMORPH, ENCBUTPININSTR1, ENCBUTPININSTR2, ENCBUTPININSTR3, ENCBUTPININSTR4, ENCBUTPININSTR5, ENCBUTPININSTR6};
uint8_t sampleButtonModifier[MAXENCODERS] = {0, 0, ENCBUTINSTR2, ENCBUTINSTR1, ENCBUTINSTR4, ENCBUTINSTR3, ENCBUTINSTR6, ENCBUTINSTR5}; //this points to each instrument sample change button modifier

boolean encoderButtonState[MAXENCODERS] = {0, 0, 0, 0, 0, 0, 0, 0};

uint8_t instrActionPins[MAXINSTRUMENTS] = {ACTIONPININSTR1, ACTIONPININSTR2, ACTIONPININSTR3, ACTIONPININSTR4, ACTIONPININSTR5, ACTIONPININSTR6}; //pins
boolean instrActionState[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
boolean permanentMute[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
uint8_t maxInstrSamples[MAXINSTRUMENTS] = {MAXINSTR1SAMPLES, MAXINSTR2SAMPLES, MAXINSTR3SAMPLES, MAXINSTR4SAMPLES, MAXINSTR5SAMPLES, MAXINSTR6SAMPLES};

//playing instrument pattern pointer
Counter instr1patternPointer(MAXINSTR1PATTERNS - 1);
Counter instr2patternPointer(MAXINSTR2PATTERNS - 1);
Counter instr3patternPointer(MAXINSTR2PATTERNS - 1);
Counter instr4patternPointer(MAXINSTR2PATTERNS - 1);
Counter instr5patternPointer(MAXINSTR2PATTERNS - 1);
Counter instr6patternPointer(MAXINSTR6PATTERNS - 1);
Counter *drumKitPatternPlaying[MAXINSTRUMENTS] = {&instr1patternPointer, &instr2patternPointer, &instr3patternPointer, &instr4patternPointer, &instr5patternPointer, &instr6patternPointer};

//playing instrument sample pointer
Counter instr1samplePointer(MAXINSTR1SAMPLES);
Counter instr2samplePointer(MAXINSTR2SAMPLES);
Counter instr3samplePointer(MAXINSTR3SAMPLES);
Counter instr4samplePointer(MAXINSTR4SAMPLES);
Counter instr5samplePointer(MAXINSTR5SAMPLES);
Counter instr6samplePointer(MAXINSTR6SAMPLES);
Counter *drumKitSamplePlaying[MAXINSTRUMENTS] = {&instr1samplePointer, &instr2samplePointer, &instr3samplePointer, &instr4samplePointer, &instr5samplePointer, &instr6samplePointer};

Rotary MOODencoder(ENCPINAMOOD, ENCPINBMOOD);
Rotary MORPHencoder(ENCPINAMORPH, ENCPINBMORPH);
Rotary instr1encoder(ENCPINAINSTR1, ENCPINBINSTR1);
Rotary instr2encoder(ENCPINAINSTR2, ENCPINBINSTR2);
Rotary instr3encoder(ENCPINAINSTR3, ENCPINBINSTR3);
Rotary instr4encoder(ENCPINAINSTR4, ENCPINBINSTR4);
Rotary instr5encoder(ENCPINAINSTR5, ENCPINBINSTR5);
Rotary instr6encoder(ENCPINAINSTR6, ENCPINBINSTR6);
Rotary *encoders[MAXENCODERS] = {&MOODencoder, &MORPHencoder, &instr1encoder, &instr2encoder, &instr3encoder, &instr4encoder, &instr5encoder, &instr6encoder};

int8_t morphingInstr = 0;
volatile int8_t stepCount = -1;
int8_t queuedEncoder = 0;

boolean updateOledBrowseMood = false;      //schedule some Oled update
boolean updateOledSelectMood = false;      //schedule some Oled update
boolean updateOledUpdateBpm = false;       //update bpm
int16_t selectedMood = 0;                  //actual selected mood
int16_t lastSelectedMood = 255;            //prevents to execute 2 times the same action
int8_t previousMood = 0;                   //previous mood name
int8_t lastMorphBarGraphValue = 127;       //0 to MAXINSTRUMENTS possible values
int8_t updateOledUpdateMorph = 0;          //schedule some Oled update
int8_t updateOledInstrPattern = -1;        //update only one instrument pattern
boolean updateOledEraseMoodName = false;   //erase mood name when change any original mood configuration
int8_t updateOledChangePlayingSample = -1; //update sample on rasp pi
int8_t updateOledUpdategateLenght = -1;    //update gate lenght on right upper corner
int8_t updateOledEraseInstrumentPattern = -1;
int8_t updateOledTapInstrumentPattern = -1;
int8_t updateOledRollbackInstrumentTap = -1;
boolean defaultOledNotActiveYet = true;

#include "patterns.h"
//byte *romPatternPoniter[6] = {&romPatternInstr1, &romPatternInstr5, &romPatternInstr5, &romPatternInstr5, &romPatternInstr5, &romPatternInstr6};
uint8_t sourcedPattern[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
boolean isCustomPattern[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};

//MOOD===================================================================================================================
#define MAXMOODS 4 //max moods
String moodKitName[MAXMOODS] = {"", "P.Labs-Choke", "P.Labs-Empty Room", "P.Labs-April23"};

//SAMPLES + PATTERN
int8_t moodKitPresetPointers[MAXMOODS][(2 * MAXINSTRUMENTS)] = {
    //s, s, s, s, s, s, p, p, p, p, p, p
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, //reserved MUTE = 1  NOT = 0
    {0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 1},
    {1, 1, 1, 1, 1, 1, 2, 3, 3, 3, 3, 2},
    {2, 2, 2, 2, 2, 1, 2, 4, 4, 4, 4, 1}};

//MORPH 6 pointer samples + 6 pointer patterns / 0=morph area 0 / 1=morph area 1
uint8_t morphArea[2][2 * MAXINSTRUMENTS];
boolean ramPatterns[2][6][MAXSTEPS];
#define RAM 0
#define ROM 1
#define MAXUNDOS MAXSTEPS
int8_t undoStack[MAXUNDOS][MAXINSTRUMENTS];
String rightCornerInfo[5] = {"pat", "smp", "bpm", "ms", "len"};

// #define MAXLOFIKICK 11
// uint8_t lofiKick[MAXLOFIKICK] = {0, 3, 4, 12, 17, 23, 25, 42, 43, 44, 45};
// #define MAXLOFIHAT 10
// uint8_t lofiHihat[MAXLOFIHAT] = {6, 16, 17, 22, 24, 33, 42, 50, 53, 62};
boolean updateOledModifierPressed = false;

void ISRfireTimer3();
void ISRfireTimer4();
void playMidi(uint8_t _note, uint8_t _velocity, uint8_t _channel);
void readEncoder();
void checkDefaultOled();
void oledWelcome();
void oledEraseBrowsedMoodName();
void oledShowBrowsedMood();
void oledShowMorphBar(int8_t _size);
void oledShowCornerInfo(uint8_t _parm, int16_t _val);
void oledEraseInstrumentPattern(uint8_t _instr);
void oledShowInstrumPattern(uint8_t _instr, boolean _source);
void oledShowPreviousMood();
void oledCustomMoodName();
void oledShowSelectedMood();
void endTriggers();
boolean sampleUpdateWindow();
boolean noOneEncoderButtonIsPressed();
boolean onlyOneEncoderButtonIsPressed(uint8_t target);
boolean twoEncoderButtonsArePressed(uint8_t _target1, uint8_t _target2);
void eraseInstrumentRam1Pattern(uint8_t _instr);
void setStepIntoRamPattern(uint8_t _instr, uint8_t _step, uint8_t _val);
void clearUndoArray(uint8_t _instr);
void setStepIntoRomPattern(uint8_t _instr, uint8_t _pattern, uint8_t _step, uint8_t _val);
void setStepIntoUndoArray(uint8_t _instr, uint8_t _step, uint8_t _val);
void setAllPatternAsOriginal();
void setThisPatternAsOriginal(uint8_t _instr);
void setThisPatternAsCustom(uint8_t _instr);
void rollbackStepIntoUndoArray(uint8_t _instr);
void copyRomPatternToRam1(uint8_t _instr, uint8_t _romPatternTablePointer);
void copyRomPatternToRomPattern(uint8_t _instr, uint8_t _source);
void copyRamPattern1ToRam0(uint8_t _instr);
void copyRamPattern0ToRam1(uint8_t _instr);
void calculateMidiArray();

void setup()
{
#ifdef DO_MIDIHW
  MIDI.begin();
#endif

#ifdef DO_SERIAL
  Serial.begin(9600);
  Serial.println("Debugging..");
#endif

  //pinMode(TRIGGERINPIN, INPUT);
  //triggerIn.attachCallOnRising(ISRfireTimer3);

  //start all encoders
  for (uint8_t i = 0; i < MAXENCODERS; i++)
    encoders[i]->begin();

  //start all encoders buttons
  for (uint8_t i = 0; i < MAXENCODERS; i++)
    pinMode(encoderButtonPins[i], INPUT_PULLUP);

  //start all Action buttons
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    pinMode(instrActionPins[i], INPUT_PULLUP);

  //start all trigger out pins
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    pinMode(triggerPins[i], OUTPUT);

  //set these counters to not cyclabe
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
  {
    drumKitPatternPlaying[i]->setCyclable(false);
    drumKitSamplePlaying[i]->setCyclable(false);
  }

  for (uint8_t undos = 0; undos < MAXUNDOS; undos++)
    for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++)
      undoStack[undos][instr] = -1;

  for (uint8_t morph = 0; morph < 2; morph++)
    for (uint8_t instr = 0; instr < (2 * MAXINSTRUMENTS); instr++)
      morphArea[morph][instr] = 0;

  for (uint8_t morph = 0; morph < 2; morph++)
    for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++)
      for (uint8_t step = 0; step < MAXSTEPS; step++)
        ramPatterns[morph][instr][step] = 0;

  //setup display
  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS))
  {
    // Address 0x3D for 128x64
    Serial.begin(9600);
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed
  }
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // white text
  display.cp437(true);
  display.setCursor(0, 0);
  display.println(F("Pantala Labs"));
  display.println(F("Taste & Flavor"));
  display.display();
  delay(2000);
  oledWelcome();
  display.display();

  //internal clock
  u_bpm = bpm2micros4ppqn(bpm);
  Timer3.attachInterrupt(ISRfireTimer3);
  Timer3.start(u_bpm);
  Timer4.attachInterrupt(ISRfireTimer4);
  Timer5.attachInterrupt(endTriggers);
}

//base clock
void ISRfireTimer3()
{
  Timer3.stop();
  u_tickInterval = micros() - u_lastTick;
  u_lastTick = micros();
  if (u_timeShift >= 0)
  {
    Timer4.start(u_timeShift);
    //    Timer4.attachInterrupt(ISRfireTimer4);
  }
  else
  {
    Timer4.start(u_tickInterval + u_timeShift);
    //    Timer4.attachInterrupt(ISRfireTimer4);
  }
  Timer3.start(bpm2micros4ppqn(bpm));
}

//shifted clock and everything to step sequencer related
void ISRfireTimer4()
{
  noInterrupts();
  gateLenghtCounter = 0;
  Timer5.start(DEFAULTGATELENGHT); //start 5ms first trigger timer
  Timer4.stop();                   //stop timer shift
  //Timer4.detachInterrupt();                                //detach procedure
  for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++) //for each instrument
    if (midiArray[instr])
      digitalWrite(triggerPins[instr], HIGH);
  if (playMidiValue != 0)
    playMidi(1, playMidiValue, MIDICHANNEL); //send midinote with binary coded triggers message
  interrupts();
  stepCount++;
  if (stepCount >= MAXSTEPS)
    stepCount = 0;
  sampleChangeWindowEndTime = (micros() + u_tickInterval - OLEDUPDATETIME);
}

//this a a proc to CONSTANTLY calculate midi array
//so when the timer interruption comes (ISRfireTimer4) , you have all data pre calculated already available
void calculateMidiArray()
{
  playMidiValue = 0;
  for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++) //for each instrument
  {                                                        //if it is not Actiond
    uint8_t chooseMorphedOrNot = 0;                        //force point to morph area 0
    if ((morphingInstr - 1) >= instr)                      //if instrument already morphed
      chooseMorphedOrNot = 1;                              // point to RAM area 1
    //if not permanently muted and not momentary muted and it is a valid step
    if (!permanentMute[instr] && !instrActionState[instr] && ramPatterns[chooseMorphedOrNot][instr][stepCount])
    {
      midiArray[instr] = true;
      playMidiValue = playMidiValue + powint(2, 5 - instr);
    }
    else
    {
      midiArray[instr] = false;
    }
  }
}
void loop()
{
  //flags if any Oled update will be necessary in this cycle
  boolean updateOled = false;

  //read all processed encoders interruptions
  queuedEncoder++;
  if (queuedEncoder == MAXENCODERS)
    queuedEncoder = 0;
  readEncoder(queuedEncoder);

  //check for all Oled updates ===============================================================
  //if new mood was selected
  if (updateOledBrowseMood)
  {
    checkDefaultOled();
    oledShowBrowsedMood();
    updateOledBrowseMood = false;
    updateOled = true;
  }
  //update right upper corner with bpm value
  else if (updateOledUpdateBpm)
  {
    updateOledUpdateBpm = false;
    oledShowCornerInfo(2, bpm);
    updateOled = true;
  }
  //update morph bar changes and there is available time to load new sample and play it
  //this could be took off if using Tsunami
  else if ((updateOledUpdateMorph != 0) && sampleUpdateWindow())
  {
    //if CLOCKWISE : morph increased , point actual instrumentPointers morph area [1]
    if (updateOledUpdateMorph == 1)
    {
      morphingInstr++;
      if (morphingInstr > (MAXINSTRUMENTS + 1))
        morphingInstr--;
      drumKitSamplePlaying[morphingInstr - 1]->setValue(morphArea[1][morphingInstr - 1]);
      drumKitPatternPlaying[morphingInstr - 1]->setValue(morphArea[1][morphingInstr - 1 + MAXINSTRUMENTS]);
      //send new sample MIDI note do PI
      playMidi(instrSampleMidiNote[morphingInstr - 1], drumKitSamplePlaying[morphingInstr - 1]->getValue(), MIDICHANNEL);
    }
    //if COUNTER-CLOCKWISE : morph decreased , point actual instrumentPointers morph area [0]
    else if (updateOledUpdateMorph == -1)
    {
      drumKitSamplePlaying[morphingInstr - 1]->setValue(morphArea[0][morphingInstr - 1]);
      drumKitPatternPlaying[morphingInstr - 1]->setValue(morphArea[0][morphingInstr - 1 + MAXINSTRUMENTS]);
      //send new sample MIDI note do PI
      playMidi(instrSampleMidiNote[morphingInstr - 1], drumKitSamplePlaying[morphingInstr - 1]->getValue(), MIDICHANNEL);
      morphingInstr--;
      if (morphingInstr < 0)
        morphingInstr = 0;
    }
    checkDefaultOled();
    oledShowMorphBar(morphingInstr);
    updateOledUpdateMorph = 0;
    updateOled = true;
  }
  //copy selected mood to morph area
  else if (updateOledSelectMood)
  {
    checkDefaultOled();
    for (uint8_t pat = 0; pat < MAXINSTRUMENTS; pat++) //reset all permanet mute
      permanentMute[pat] = 0;
    oledShowPreviousMood();
    oledShowSelectedMood();
    setAllPatternAsOriginal();
    for (uint8_t pat = 0; pat < MAXINSTRUMENTS; pat++) // search for any BKPd pattern
    {
      gateLenghtSize[pat] = 0;      //clear all gate lengh sizes
      if (sourcedPattern[pat] != 0) //if any pattern was changed , restore it from bkp before do anything
      {
        copyRomPatternToRomPattern(pat, BKPPATTERN, sourcedPattern[pat]); //copy BKPd pattern to its original place
        sourcedPattern[pat] = 0;
      }
    }
    //copy selected mood samples and patterns pointers to or from morph area
    for (uint8_t i = 0; i < MAXINSTRUMENTS; i++) //for each instrument
    {
      //update morph area 0
      morphArea[0][i] = drumKitSamplePlaying[i]->getValue();                   //copy playing sample to morphArea 0
      morphArea[0][i + MAXINSTRUMENTS] = drumKitPatternPlaying[i]->getValue(); //copy playing pattern to morphArea 0
      copyRamPattern1ToRam0(i);                                                //copy ramPattern area 1 to ramPattern area 0
      //update morph area 1
      //sample
      if (moodKitPresetPointers[selectedMood][i] != -1)           //if selected mood sample set NOT to REUSE
        morphArea[1][i] = moodKitPresetPointers[selectedMood][i]; //copy new selected mood sample to morphArea 1
      else                                                        //else
        morphArea[1][i] = morphArea[0][i];                        //use actual playing sample
      //pattern
      if (moodKitPresetPointers[selectedMood][i + MAXINSTRUMENTS] != -1) //if selected mood pattern set NOT to REUSE
      {
        morphArea[1][i + MAXINSTRUMENTS] = moodKitPresetPointers[selectedMood][i + MAXINSTRUMENTS]; //copy selected instrument pattern to morphArea 1
        copyRomPatternToRam1(i, moodKitPresetPointers[selectedMood][i + MAXINSTRUMENTS]);           //copy selected instrument pattern to RAM
      }
      else
      {                                                                      //else
        morphArea[1][i + MAXINSTRUMENTS] = morphArea[0][i + MAXINSTRUMENTS]; //use actual playing pattern
        copyRamPattern0ToRam1(i);                                            //copy IN USE ramPattern area 0 to ramPattern area 1
      }
    }
    previousMood = selectedMood;
    oledShowMorphBar(-1);
    morphingInstr = 0;
    updateOledSelectMood = false;
    updateOled = true;
  }

  //if only one intrument pattern changed
  else if (updateOledInstrPattern != -1)
  {
    oledShowInstrumPattern(updateOledInstrPattern, RAM);
    oledShowCornerInfo(0, morphArea[1][updateOledInstrPattern + MAXINSTRUMENTS]);
    if (updateOledEraseMoodName)
    {
      updateOledEraseMoodName = false;
      oledCustomMoodName();
    }
    updateOled = true;
    updateOledInstrPattern = -1;
  }
  //if sampler changed and there is available time to update screen
  else if ((updateOledChangePlayingSample != -1) && sampleUpdateWindow())
  {
    //send midi to pi to change to new sample
    playMidi(instrSampleMidiNote[updateOledChangePlayingSample], drumKitSamplePlaying[updateOledChangePlayingSample]->getValue(), MIDICHANNEL);
    if (updateOledEraseMoodName)
    {
      updateOledEraseMoodName = false;
      oledCustomMoodName();
    }
    oledShowCornerInfo(1, drumKitSamplePlaying[updateOledChangePlayingSample]->getValue());
    updateOled = true;
    updateOledChangePlayingSample = -1;
  }
  //if time shift changed
  else if (updateOledUpdateTimeShift)
  {
    oledShowCornerInfo(3, (u_timeShift - 100) / 1000);
    updateOled = true;
    updateOledUpdateTimeShift = false;
  }
  //if gate lenght changed
  else if (updateOledUpdategateLenght != -1)
  {
    oledShowCornerInfo(4, updateOledUpdategateLenght);
    updateOled = true;
    updateOledUpdategateLenght = -1;
  }
  //if erase pattern
  else if (updateOledEraseInstrumentPattern != -1)
  {
    oledEraseInstrumentPattern(updateOledEraseInstrumentPattern); //clear pattern from display
    updateOled = true;
    updateOledEraseInstrumentPattern = -1;
  }
  //if step should be tapped
  else if (updateOledTapInstrumentPattern != -1)
  {
    oledShowInstrumPattern(updateOledTapInstrumentPattern, RAM); //update Oled with new inserted step
    updateOled = true;
    updateOledTapInstrumentPattern = -1;
  }
  else if (updateOledRollbackInstrumentTap != -1)
  {
    oledShowInstrumPattern(updateOledRollbackInstrumentTap, RAM); //update Oled with removed step
    updateOled = true;
    updateOledRollbackInstrumentTap = -1;
  }

  //if any Oled update
  if (updateOled)
    display.display();

  //read interface inputs ===========================================================================================================
  //read all encoders buttons (invert boolean state to make easy future comparation)
  for (int8_t i = 0; i < MAXENCODERS; i++)
    encoderButtonState[i] = !digitalRead(encoderButtonPins[i]);

  //if new mood was selected....copy reference tables or create new mood in the morph area
  if (onlyOneEncoderButtonIsPressed(ENCBUTMOOD) && interfaceEvent.debounced())
  {
    interfaceEvent.debounce(1000); //block any other interface event
    updateOledSelectMood = true;
  }

  //read all action buttons (invert boolean state to make easy future comparation)
  for (int8_t i = 0; i < MAXINSTRUMENTS; i++)
  {
    instrActionState[i] = !digitalRead(instrActionPins[i]); //read action instrument button

    //if any pattern should be erased
    //morph encoder + instrument modifier + action button + not custom pattern
    if (twoEncoderButtonsArePressed(MORPHENCODER, i + 2) && instrActionState[i] && !isCustomPattern[i] && interfaceEvent.debounced())
    {
      //if this pattern wasnt BKPd yet
      if (sourcedPattern[i] == 0)
      {
        copyRomPatternToRomPattern(i, drumKitPatternPlaying[i]->getValue(), BKPPATTERN); //save actual pattern into bkp area 0
        sourcedPattern[i] = drumKitPatternPlaying[i]->getValue();                        //save actual playing pattern as BKPd
        setThisPatternAsCustom(i);
      }
      copyRomPatternToRomPattern(i, 1, drumKitPatternPlaying[i]->getValue()); //copy empty pattern to actual pattern
      eraseInstrumentRam1Pattern(i);
      interfaceEvent.debounce(1000);
      updateOledEraseInstrumentPattern = i;
    }
    //if any tap should be rollbacked
    //morph encoder + instrument modifier + action button + custom pattern
    else if (twoEncoderButtonsArePressed(MORPHENCODER, i + 2) && instrActionState[i] && isCustomPattern[i] && interfaceEvent.debounced())
    {
      //if there was any rollback available
      if (undoStack[0][i] != -1)
      {
        setStepIntoRomPattern(i, drumKitPatternPlaying[i]->getValue(), undoStack[0][i], 0); //insert new step into Rom pattern
        setStepIntoRamPattern(i, undoStack[0][i], 0);                                       //insert new step into Ram 1 pattern
        rollbackStepIntoUndoArray(i);                                                       //rollback the last saved step
        oledShowInstrumPattern(i, RAM);                                                     //update Oled with new inserted step
        updateOled = true;
        updateOledRollbackInstrumentTap = i;
        interfaceEvent.debounce(200);
      }
    }
    //if any pattern should be permanently muted
    //morph encoder + action button
    else if (onlyOneEncoderButtonIsPressed(MORPHENCODER) && instrActionState[i] && interfaceEvent.debounced())
    {
      permanentMute[i] = !permanentMute[i];
      interfaceEvent.debounce(1000);
    }
    //if any pattern should be tapped
    //one instrument modifier + action button
    else if (onlyOneEncoderButtonIsPressed(i + 2) && instrActionState[i] && interfaceEvent.debounced())
    {
      int8_t updateOledTapStep;
      if (micros() < (u_lastTick + (u_tickInterval >> 1))) //if we are still in the same step
      {
        //tap this step
        updateOledTapStep = stepCount;
        playMidi(1, powint(2, 5 - i), MIDICHANNEL); //send midinote to play only this instrument
      }
      else
      {
        //tap saved to next step
        int8_t nextStep = stepCount;
        nextStep++;
        if (nextStep >= MAXSTEPS)
          nextStep = 0;
        updateOledTapStep = nextStep;
        //no need to send midinote to play this instrument because it will be saved to next step cycle
      }
      //if this pattern isnt bkp´d yet
      if (sourcedPattern[i] == 0)
      {
        copyRomPatternToRomPattern(i, drumKitPatternPlaying[i]->getValue(), BKPPATTERN); //save actual pattern into bkp area 0
        sourcedPattern[i] = drumKitPatternPlaying[i]->getValue();                        //save actual playing pattern as BKPd
      }
      setStepIntoRomPattern(i, drumKitPatternPlaying[i]->getValue(), updateOledTapStep, 1); //insert new step into Rom pattern
      setStepIntoRamPattern(i, updateOledTapStep, 1);                                       //insert new step into Ram 1 pattern
      setStepIntoUndoArray(i, updateOledTapStep);
      setThisPatternAsCustom(i);
      updateOledTapInstrumentPattern = i;
      interfaceEvent.debounce(u_tickInterval / 1000);
    }
  }
  calculateMidiArray();
}

//allows to change samples only upo to after 2/3 of the tick interval
//to avoid to change sample the same time it was triggered
boolean sampleUpdateWindow()
{
  return (micros() < sampleChangeWindowEndTime);
}

//close all trigger
//  _ ______ _______ _______ _______
//_| |      |       |       |       |______
void endTriggers()
{
  //compare all triggers times
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
  {
    if (gateLenghtSize[i] <= gateLenghtCounter)
      digitalWrite(triggerPins[i], LOW);
  }
  //prepare to the next gate verification
  gateLenghtCounter++;
  if (gateLenghtCounter > MAXGATELENGHTS) //if all step steps ended
  {
    Timer5.stop(); //stops trigger timer
    gateLenghtCounter = 0;
  }
  else
  {
    Timer5.stop();                    //stops trigger timer
    Timer5.start(EXTENDEDGATELENGHT); //start 80ms triggers timers
  }
}

//read queued encoder status
void readEncoder(uint8_t _queued)
{
  unsigned char result = encoders[_queued]->process();
  if ((result == 16) || (result == 32))
  {
    int8_t encoderChange = 0;
    if (result == 16)
      encoderChange = 1;
    else
      encoderChange = -1;

    switch (_queued)
    {
    case MOODENCODER:
      selectedMood += encoderChange;
      selectedMood = constrain(selectedMood, 0, MAXMOODS - 1);
      //saves CPU if mood changed but still in the same position , in the start and in the end of the list
      if (lastSelectedMood != selectedMood)
      {
        updateOledBrowseMood = true;
        lastSelectedMood = selectedMood;
      }
      break;

    case MORPHENCODER:
      //if morph button AND morph button are pressed and MORPH rotate change timer shift
      if (twoEncoderButtonsArePressed(ENCBUTMOOD, ENCBUTMORPH))
      {
        u_timeShift += encoderChange * u_timeShiftStep;
        u_timeShift = constrain(u_timeShift, -u_timeShiftLimit, u_timeShiftLimit);
        updateOledUpdateTimeShift = true;
      }
      //if morph button is pressed and MORPH rotate CHANGE BPM
      else if (onlyOneEncoderButtonIsPressed(ENCBUTMORPH))
      {
        updateOledUpdateBpm = true;
        bpm += encoderChange;
        u_bpm = bpm2micros4ppqn(bpm);
      }
      else
      { //or else , morph change
        //CLOCKWISE : morph increased , point actual instrumentPointers morph area [1]
        if (encoderChange == 1)
        {
          if (morphingInstr < MAXINSTRUMENTS) //if there was one more morphing position available
            updateOledUpdateMorph = encoderChange;
        }
        //if COUNTER-CLOCKWISE : morph decreased , point actual instrumentPointers morph area [0]
        else if (encoderChange == -1)
        {
          if (morphingInstr > 0) //if there was one more morphing position available
            updateOledUpdateMorph = encoderChange;
        }
      }
      break;
    default:
      //all other instrument encoders
      uint8_t _instrum = _queued - 2;
      //if only one ASSOCIATED modifier pressed
      if (onlyOneEncoderButtonIsPressed(sampleButtonModifier[_queued]))
      {
        if (encoderChange == -1)
          drumKitSamplePlaying[_instrum]->reward();
        else
          drumKitSamplePlaying[_instrum]->advance();
        // if ((_instrum) <= (morphingInstr))
        morphArea[1][_instrum] = drumKitSamplePlaying[_instrum]->getValue();
        // else
        //   morphArea[0][_instrum] = drumKitSamplePlaying[_instrum]->getValue();
        updateOledChangePlayingSample = _instrum;
        updateOledEraseMoodName = true;
      }
      //if only SAME button pressed , change gate lenght
      else if (onlyOneEncoderButtonIsPressed(_queued))
      {
        gateLenghtSize[_instrum] += encoderChange;
        gateLenghtSize[_instrum] = constrain(gateLenghtSize[_instrum], 0, MAXGATELENGHTS);
        updateOledUpdategateLenght = _instrum;
      }
      else if (noOneEncoderButtonIsPressed()) //if no modifier pressed , only change instrument pattern , schedule event
      {
        //if this pattern was tapped and BKPd , restore
        if (sourcedPattern[_instrum] != 0)
        {
          copyRomPatternToRomPattern(_instrum, BKPPATTERN, drumKitPatternPlaying[_instrum]->getValue()); //reverse BKPd pattern to rom area
          clearUndoArray(_instrum);
          setThisPatternAsOriginal(_instrum);
        }
        sourcedPattern[_instrum] = 0;
        int8_t nextRomPatternTablePointer = morphArea[1][_instrum + MAXINSTRUMENTS]; //calculate new pattern pointer for this instrument
        nextRomPatternTablePointer += encoderChange;
        nextRomPatternTablePointer = constrain(nextRomPatternTablePointer, 1, maxInstrSamples[_instrum] - 1); //constrain sample pointer to1 and max samples per instrument
        morphArea[1][_instrum + MAXINSTRUMENTS] = nextRomPatternTablePointer;                                 //update destination morph area with the new pattern value
        if (_instrum < morphingInstr)                                                                         //if instrument not morphed , force play the selected pattern
          drumKitPatternPlaying[_instrum]->setValue(morphArea[1][_instrum + MAXINSTRUMENTS]);                 //set new value
        copyRomPatternToRam1(_instrum, morphArea[1][_instrum + MAXINSTRUMENTS]);                              //copy selected instrument pattern to RAM
        updateOledInstrPattern = _instrum;                                                                    //flags to update this instrument on Display
        updateOledEraseMoodName = true;
      }
      break;
    }
  }
}

//check if default Oled wasnt show yet
void checkDefaultOled()
{
  if (defaultOledNotActiveYet)
  {
    defaultOledNotActiveYet = false;
    display.clearDisplay();
    display.drawRect(0, 0, 60, TEXTLINE_HEIGHT - 2, WHITE);
    oledShowBrowsedMood();
    oledShowMorphBar(-1);
  }
}

void oledWelcome()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("<-- select your mood"));
  display.println(F("    and morph it -->"));
}

void oledShowMorphBar(int8_t _size) //update morphing status / 6 steps of 10 pixels each
{
  if (lastMorphBarGraphValue != _size)
  {
    display.fillRect(1, 1, 58, TEXTLINE_HEIGHT - 4, BLACK);
    for (int8_t i = 0; i < _size; i++)
      display.fillRect(10 * i, 0, 10, TEXTLINE_HEIGHT - 2, WHITE);
    lastMorphBarGraphValue = _size;
  }
}

void oledShowCornerInfo(uint8_t _parm, int16_t _val) //update Oled right upper corner with the actual sample or pattern number
{
  display.fillRect(70, 0, DISPLAY_WIDTH - 70, TEXTLINE_HEIGHT, BLACK);
  display.setCursor(70, 0);
  display.setTextColor(WHITE);
  display.print(rightCornerInfo[_parm]);
  display.print(":");
  switch (_parm)
  {
  case 0:
  case 1:
  case 2:
  case 3:
    display.print(_val);
    break;
  case 4:
    display.print((DEFAULTGATELENGHT + (gateLenghtSize[_val] * EXTENDEDGATELENGHT)) / 1000);
  }
}

void oledShowPreviousMood()
{
  display.fillRect(0, TEXTLINE_HEIGHT, DISPLAY_WIDTH, TEXTLINE_HEIGHT, BLACK);
  display.setTextColor(WHITE);
  display.setCursor(0, TEXTLINE_HEIGHT);    //set cursor position
  display.print(moodKitName[previousMood]); //print previous mood name
}

void oledShowSelectedMood()
{
  display.fillRect(0, 2 * TEXTLINE_HEIGHT, DISPLAY_WIDTH, TEXTLINE_HEIGHT, BLACK);
  display.setTextColor(WHITE);
  display.setCursor(0, 2 * TEXTLINE_HEIGHT); //set cursor position
  display.print(moodKitName[selectedMood]);  //print selected mood name
}
void oledCustomMoodName()
{
  oledEraseBrowsedMoodName();
  display.setTextColor(WHITE);
  display.setCursor(0, 3 * TEXTLINE_HEIGHT); //set cursor position
  display.print("Custom");                   //print Custom name
}
void oledEraseBrowsedMoodName()
{
  display.fillRect(0, 3 * TEXTLINE_HEIGHT, DISPLAY_WIDTH, TEXTLINE_HEIGHT - 1, BLACK);
}

void oledShowBrowsedMood() //update almost all bottom Oled area with the name of the selected mood and all 6 available instruments
{
  oledEraseBrowsedMoodName();
  display.setTextColor(WHITE);
  display.setCursor(0, 3 * TEXTLINE_HEIGHT);              //set cursor position
  display.print(moodKitName[selectedMood]);               //print mood name
  for (int8_t instr = 0; instr < MAXINSTRUMENTS; instr++) //for each instrument
    oledShowInstrumPattern(instr, ROM);
}

void oledShowInstrumPattern(uint8_t _instr, boolean _source)
{
  boolean mustPrintStep = false;
  uint8_t targetPattern = moodKitPresetPointers[selectedMood][_instr + MAXINSTRUMENTS];
  oledEraseInstrumentPattern(_instr);
  display.setTextColor(WHITE);
  for (int8_t step = 0; step < (MAXSTEPS - 1); step++) //for each step
  {
    mustPrintStep = false;
    if ((_source == RAM) && (ramPatterns[1][_instr][step])) //if source = RAM
    {
      mustPrintStep = true;
    }
    //source = ROM and a valid pattern > 0
    else if ((_source == ROM) && (targetPattern > 0))
    {
      if (_instr == KICKCHANNEL)
      {
        if (romPatternInstr1[targetPattern][step]) //if it is an active step
          mustPrintStep = true;
      }
      else if (_instr == SNAKERCHANNEL)
      {
        if (romPatternInstr2[targetPattern][step]) //if it is an active step
          mustPrintStep = true;
      }
      else if (_instr == CLAPCHANNEL)
      {
        if (romPatternInstr3[targetPattern][step]) //if it is an active step
          mustPrintStep = true;
      }
      else if (_instr == HATCHANNEL)
      {
        if (romPatternInstr4[targetPattern][step]) //if it is an active step
          mustPrintStep = true;
      }
      else if (_instr == OHHRIDECHANNEL)
      {
        if (romPatternInstr5[targetPattern][step]) //if it is an active step
          mustPrintStep = true;
      }
      else
      {
        if (romPatternInstr6[targetPattern][step]) //if it is an active step
          mustPrintStep = true;
      }
    }
    if (mustPrintStep)
      display.fillRect(step * 2, DOTGRIDINIT + (_instr * GRIDPATTERNHEIGHT), 2, GRIDPATTERNHEIGHT - 1, WHITE);
  }
}

//erase exatctly one line pattern
void oledEraseInstrumentPattern(uint8_t _instr)
{
  display.fillRect(0, DOTGRIDINIT + (_instr * GRIDPATTERNHEIGHT), DISPLAY_WIDTH, GRIDPATTERNHEIGHT - 1, BLACK);
}

void playMidi(uint8_t _note, uint8_t _velocity, uint8_t _channel)
{
#ifdef DO_MIDIHW
  MIDI.sendNoteOn(_note, _velocity, _channel);
#endif

#ifdef DO_MIDIUSB
  noteOn(_channel, _note, _velocity); // Channel 0, middle C, normal velocity
  MidiUSB.flush();
#endif
}

//verify if NO ONE encoder button is pressed
boolean noOneEncoderButtonIsPressed()
{
  for (uint8_t i = 0; i < MAXENCODERS; i++) //search all encoder buttons
    if (encoderButtonState[i])              //it is pressed
      return false;
  //if all tests where OK
  return true;
}

//verify if ONLY ONE encoder button is pressed
boolean onlyOneEncoderButtonIsPressed(uint8_t _target)
{
  if (encoderButtonState[_target]) //if asked button is pressed
  {
    for (uint8_t i = 0; i < MAXENCODERS; i++)      //search all encoder buttons
      if ((i != _target) && encoderButtonState[i]) //encoder button is not the asked one and it is pressed , so there are 2 encoder buttons pressed
        return false;
  }
  else //if not pressed , return false
    return false;
  //if all tests where OK
  return true;
}

//verify if TWO encoder button is pressed
boolean twoEncoderButtonsArePressed(uint8_t _target1, uint8_t _target2)
{
  if (encoderButtonState[_target1] && encoderButtonState[_target2]) //if two asked buttons are pressed
  {
    for (uint8_t i = 0; i < MAXENCODERS; i++)                          //search all encoder buttons
      if ((i != _target1) && (i != _target2) && encoderButtonState[i]) //encoder button is not any of asked and it is pressed , so there is a third encoder buttons pressed
        return false;
  }
  else //if not pressed , return false
    return false;
  //if all tests where OK
  return true;
}

//clear pattern from ram 1 area
void eraseInstrumentRam1Pattern(uint8_t _instr)
{
  for (int8_t step = 0; step < MAXSTEPS; step++)
    ramPatterns[1][_instr][step] = 0;
}

//save tapped step into respective rom table
void setStepIntoRomPattern(uint8_t _instr, uint8_t _pattern, uint8_t _step, uint8_t _val)
{
  if (_instr == KICKCHANNEL)
    romPatternInstr1[_pattern][_step] = _val;
  else if (_instr == SNAKERCHANNEL)
    romPatternInstr2[_pattern][_step] = _val;
  else if (_instr == CLAPCHANNEL)
    romPatternInstr3[_pattern][_step] = _val;
  else if (_instr == HATCHANNEL)
    romPatternInstr4[_pattern][_step] = _val;
  else if (_instr == OHHRIDECHANNEL)
    romPatternInstr5[_pattern][_step] = _val;
  else
    romPatternInstr6[_pattern][_step] = _val;
}

void setStepIntoRamPattern(uint8_t _instr, uint8_t _step, uint8_t _val)
{
  ramPatterns[1][_instr][_step] = _val; //save tapped step
}

void clearUndoArray(uint8_t _instr)
{
  for (uint8_t i = 0; i < MAXUNDOS; i++)
    undoStack[i][_instr] = -1;
}

void setStepIntoUndoArray(uint8_t _instr, uint8_t _step)
{
  for (uint8_t i = (MAXUNDOS - 1); i > 0; i--)
    undoStack[i][_instr] = undoStack[i - 1][_instr]; //move all values ahead to open space on stack
  undoStack[0][_instr] = _step;
}

void setAllPatternAsOriginal()
{
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    setThisPatternAsOriginal(i);
}

void setThisPatternAsOriginal(uint8_t _instr)
{
  isCustomPattern[_instr] = 0;
}

void setThisPatternAsCustom(uint8_t _instr)
{
  isCustomPattern[_instr] = 1;
}

void rollbackStepIntoUndoArray(uint8_t _instr)
{
  for (uint8_t i = 0; i < (MAXUNDOS - 1); i++)
    undoStack[i][_instr] = undoStack[i + 1][_instr]; //move all values ahead to open space on stack
  undoStack[(MAXUNDOS - 1)][_instr] = -1;
}

uint8_t lastCopied_instr;
uint8_t lastCopied_PatternTablePointer;
//copy selected instrument pattern from ROM to RAM
void copyRomPatternToRam1(uint8_t _instr, uint8_t _romPatternTablePointer)
{
  //code repeat protection
  if ((lastCopied_instr == _instr) && (lastCopied_PatternTablePointer == _romPatternTablePointer))
    return;
  lastCopied_instr = _instr;
  lastCopied_PatternTablePointer = _romPatternTablePointer;
  for (uint8_t i = 0; i < MAXSTEPS; i++)
  {
    if (_instr == KICKCHANNEL)
      ramPatterns[1][_instr][i] = romPatternInstr1[_romPatternTablePointer][i];
    else if (_instr == SNAKERCHANNEL)
      ramPatterns[1][_instr][i] = romPatternInstr2[_romPatternTablePointer][i];
    else if (_instr == CLAPCHANNEL)
      ramPatterns[1][_instr][i] = romPatternInstr3[_romPatternTablePointer][i];
    else if (_instr == HATCHANNEL)
      ramPatterns[1][_instr][i] = romPatternInstr4[_romPatternTablePointer][i];
    else if (_instr == OHHRIDECHANNEL)
      ramPatterns[1][_instr][i] = romPatternInstr5[_romPatternTablePointer][i];
    else
      ramPatterns[1][_instr][i] = romPatternInstr6[_romPatternTablePointer][i];
  }
}

//copy selected instrument pattern from ROM to ROM
void copyRomPatternToRomPattern(uint8_t _instr, uint8_t _source, uint8_t _target)
{
  for (uint8_t i = 0; i < MAXSTEPS; i++)
  {
    if (_instr == KICKCHANNEL)
      romPatternInstr1[_target][i] = romPatternInstr1[_source][i];
    else if (_instr == SNAKERCHANNEL)
      romPatternInstr2[_target][i] = romPatternInstr2[_source][i];
    else if (_instr == CLAPCHANNEL)
      romPatternInstr3[_target][i] = romPatternInstr3[_source][i];
    else if (_instr == HATCHANNEL)
      romPatternInstr4[_target][i] = romPatternInstr4[_source][i];
    else if (_instr == OHHRIDECHANNEL)
      romPatternInstr5[_target][i] = romPatternInstr5[_source][i];
    else
      romPatternInstr6[_target][i] = romPatternInstr6[_source][i];
  }
}

//copy ramPattern area 1 to ramPattern area 0
void copyRamPattern1ToRam0(uint8_t _instr)
{
  for (uint8_t i = 0; i < MAXSTEPS; i++)
    ramPatterns[0][_instr][i] = ramPatterns[1][_instr][i];
}

//copy ramPattern area 0 to ramPattern area 1
void copyRamPattern0ToRam1(uint8_t _instr)
{
  for (uint8_t i = 0; i < MAXSTEPS; i++)
    ramPatterns[1][_instr][i] = ramPatterns[0][_instr][i];
}

#ifdef DO_MIDIUSB
void noteOn(byte channel, byte pitch, byte velocity)
{
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity)
{
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void controlChange(byte channel, byte control, byte value)
{
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

#endif