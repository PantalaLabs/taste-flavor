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

#define DO_SERIAL true

#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();

#include <PantalaDefines.h>
#include <EventDebounce.h>
//#include <DigitalInput.h>
#include <Counter.h>
#include <Rotary.h>
#include <DueTimer.h>

#define I2C_ADDRESS 0x3C
#define Display_WIDTH 128 // OLED display width, in pixels
#define Display_HEIGHT 64 // OLED display height, in pixels
#define TEXTLINE_HEIGHT 9 // OLED display height, in pixels

//adafruit
#include <Adafruit_SSD1306.h>
#define OLED_RESET 47 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(Display_WIDTH, Display_HEIGHT, &Wire, OLED_RESET);

#define MAXSTEPS 64      //max step sequence
#define MYCHANNEL 1      //midi channel for data transmission
#define MAXENCODERS 8    //total of encoders
#define MOODENCODER 0    //total of sample encoders
#define MORPHENCODER 1   //total of sample encoders
#define MAXINSTRUMENTS 6 //total of sample encoders
#define MIDICHANNEL 1    //standart drum midi channel
#define MOODMIDINOTE 20  //midi note to mood message
#define MORPHMIDINOTE 21 //midi note to morph message
#define KICKCHANNEL 0
#define SNAKERCHANNEL 1 //snare + shaker
#define CLAPCHANNEL 2
#define HATCHANNEL 3
#define OHHRIDECHANNEL 4 //open hi hat + ride
#define PERCCHANNEL 5
#define MAXINSTR1PATTERNS 17
#define MAXINSTR2PATTERNS 27
#define MAXINSTR3PATTERNS 27
#define MAXINSTR4PATTERNS 27
#define MAXINSTR5PATTERNS 27
#define MAXINSTR6PATTERNS 19
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

uint32_t lastTick;
uint32_t tickInterval;
EventDebounce interfaceEvent(200); //min time in ms to accept another interface event

uint8_t instrSampleMidiNote[MAXINSTRUMENTS] = {10, 11, 12, 13, 14, 15};  //midi note to sample message
uint8_t instrPatternMidiNote[MAXINSTRUMENTS] = {20, 21, 22, 23, 24, 25}; //midi note to pattern message
uint8_t triggerPins[MAXINSTRUMENTS] = {TRIGOUTPIN1, TRIGOUTPIN2, TRIGOUTPIN3, TRIGOUTPIN4, TRIGOUTPIN5, TRIGOUTPIN6};
//encoders buttons : mood, morph, instruments
uint8_t encoderButtonPins[MAXENCODERS] = {ENCBUTPINMOOD, ENCBUTPINMORPH, ENCBUTPININSTR1, ENCBUTPININSTR2, ENCBUTPININSTR3, ENCBUTPININSTR4, ENCBUTPININSTR5, ENCBUTPININSTR6};
uint8_t sampleButtonModifier[MAXINSTRUMENTS] = {ENCBUTINSTR2, ENCBUTINSTR1, ENCBUTINSTR4, ENCBUTINSTR3, ENCBUTINSTR6, ENCBUTINSTR5}; //this points to each instrument sample change button modifier

boolean encoderButtonState[MAXENCODERS] = {0, 0, 0, 0, 0, 0, 0, 0};

uint8_t instrActionPins[MAXINSTRUMENTS] = {ACTIONPININSTR1, ACTIONPININSTR2, ACTIONPININSTR3, ACTIONPININSTR4, ACTIONPININSTR5, ACTIONPININSTR6}; //pins
boolean instrActionState[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
boolean permanentMute[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};

//playing instrument pattern pointer
Counter instr1patternPointer(MAXINSTR1PATTERNS - 1);
Counter instr2patternPointer(MAXINSTR2PATTERNS - 1);
Counter instr3patternPointer(MAXINSTR2PATTERNS - 1);
Counter instr4patternPointer(MAXINSTR2PATTERNS - 1);
Counter instr5patternPointer(MAXINSTR2PATTERNS - 1);
Counter instr6patternPointer(MAXINSTR6PATTERNS - 1);
Counter *drumKitPatternPlaying[MAXINSTRUMENTS] = {&instr1patternPointer, &instr2patternPointer, &instr3patternPointer, &instr4patternPointer, &instr5patternPointer, &instr6patternPointer};

//playing instrument sample pointer
Counter instr1samplePointer(126);
Counter instr2samplePointer(126);
Counter instr3samplePointer(126);
Counter instr4samplePointer(126);
Counter instr5samplePointer(126);
Counter instr6samplePointer(126);
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

Counter morphingInstrument(MAXINSTRUMENTS);
Counter stepCounter(MAXSTEPS - 1);
Counter encoderQueue(MAXENCODERS - 1);

volatile boolean flagAdvanceStepCounter = false;
boolean flagUpdateStepLenght = false;

int16_t selectedMood = 0;
int16_t lastSelectedMood = 255;      //prevents to execute 2 times the same action
int8_t previousMood = 0;             //previous mood name
int8_t lastMorphBarGraphValue = 127; //0 to MAXINSTRUMENTS possible values
boolean flagBrowseMood = false;      //schedule some Display update
int8_t flagUpdateMorph = 0;          //schedule some Display update
boolean flagSelectMood = false;      //schedule some Display update
int8_t flagUpdateThisInstrPattern = -1;
int8_t flagNextRomPatternTablePointer = -1;
int8_t flagUpdateSampleNumber = -1;
int8_t flagEraseInstrumentPattern = -1;
int8_t flagTapInstrumentPattern = -1;
int8_t flagRollbackInstrumentTap = -1;
int8_t flagTapStep = -1;
boolean defaultDisplayNotActiveYet = true;

#include "patterns.h"
//byte *romPatternPoniter[6] = {&romPatternInstr1, &romPatternInstr5, &romPatternInstr5, &romPatternInstr5, &romPatternInstr5, &romPatternInstr6};
uint8_t flagBKPdPatternFrom[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
boolean flagCustomPattern[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};

//MOOD===================================================================================================================
#define MAXMOODS 5 //max moods
String moodKitName[MAXMOODS] = {"", "4x4 mood 1", "4x4 mood 2", "4x4 mood 3", "4x4 mood 4"};

//SAMPLES + PATTERN
int8_t moodKitPresetPointers[MAXMOODS][(2 * MAXINSTRUMENTS)] = {
    //s, s, s, s, s, s, p, p, p, p, p, p
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {10, 4, 5, 3, 3, 1, 2, 3, 5, 4, 3, 1},
    {9, 4, 15, 3, 13, 3, 2, 3, 6, 2, 3, 1},
    {3, 5, 15, 3, 18, 2, 2, 3, 6, 2, 2, 1},
    {15, 5, 15, 3, 18, 2, 2, 3, 6, 0, 4, 2}};

// {18, 2, 3, 3, 13, 1},/ {3, 3, 6, 1, 0, 5},
// {18, 2, 3, 3, 0, 1},{3, 3, 9, 6, 0, 0},
// {21, 4, 4, 3, 13, 2}, {3, 3, 9, 0, 0, 0},
// {27, 5, 0, 3, 13, 3}, {5, 3, 8, 3, 3, 8},
// {30, 5, 18, 3, 17, 2},{3, 1, 9, 6, 0, 0},
// {30, 0, 14, 3, 14, 1},{3, 1, 9, 6, 3, 0},
// {30, 0, 14, 3, 14, 4},/ {3, 1, 9, 6, 0, 0},
// {30, 0, 14, 3, 14, 3}, {3, 0, 9, 0, 4, 0},
// {24, 6, 13, 3, 6, 2}, {7, 10, 11, 0, 4, 0},
// {8, 8, 13, 3, 5, 0}, {8, 10, 2, 0, 0, 0},
// {12, 25, 17, 3, 20, 1}, {9, 13, 12, 2, 0, 0},
// {3, 13, 16, 3, 17, 4}, {10, 16, 15, 14, 0, 0},
// {35, 15, 8, 3, 7, 3},{11, 19, 18, 17, 0, 0},
// {45, 11, 18, 4, 14, 1}, {12, 20, 21, 0, 0, 5},
// {41, 16, 12, 4, 7, 2}, {13, 2, 22, 0, 6, 0},
// {65, 22, 28, 4, 3, 4},{14, 23, 22, 0, 6, 0},
// {-1, -1, -1, -1, 6, -1}, {15, 24, 25, 0, 0, 0},

//MORPH 6 pointer samples + 6 pointer patterns / 0=morph area 0 / 1=morph area 1
uint8_t morphArea[2][2 * MAXINSTRUMENTS];
boolean ramPatterns[2][6][MAXSTEPS];
#define RAM 0
#define ROM 1
#define MAXUNDOS MAXSTEPS
int8_t undoStack[MAXUNDOS][MAXINSTRUMENTS];

// #define MAXLOFIKICK 11
// uint8_t lofiKick[MAXLOFIKICK] = {0, 3, 4, 12, 17, 23, 25, 42, 43, 44, 45};
// #define MAXLOFIHAT 10
// uint8_t lofiHihat[MAXLOFIHAT] = {6, 16, 17, 22, 24, 33, 42, 50, 53, 62};
uint32_t stepLenght = 125000;
boolean flagModifierPressed = false;

volatile uint8_t playMidiValue = 0;

void ISRtriggerIn();
void playMidi(uint8_t _note, uint8_t _velocity, uint8_t _channel);
void readEncoder();
void checkDefaultDisplay();
void Displaywelcome();
void displayShowBrowsedMood();
void displayShowMorphBar(int8_t _size);
void DisplayshowSampleOrPatternNumber(boolean _smp, int8_t _val);
void displayBlackInstrumentPattern(uint8_t _instr);
void displayShowInstrumPattern(uint8_t _instr, boolean _source);
void displayShowPreviousMood();
void endTriggers();
boolean oneEncoderButtonPressed(uint8_t target);
boolean twoEncoderButtonsPressed(uint8_t target1, uint8_t target2);
void clearInstrumentRam1Pattern(uint8_t _instr);
void setStepIntoRamPattern(uint8_t _instr, uint8_t _step, uint8_t _val);
void setStepIntoRomPattern(uint8_t _instr, uint8_t _pattern, uint8_t _step, uint8_t _val);
void setStepIntoUndoArray(uint8_t _instr, uint8_t _step, uint8_t _val);
void rollbackStepIntoUndoArray(uint8_t _instr);
void copyRomPatternToRam1(uint8_t _instr, uint8_t _romPatternTablePointer);
void copyRomPatternToRomPattern(uint8_t _instr, uint8_t _source);
void copyRamPattern1ToRam0(uint8_t _instr);
void copyRamPattern0ToRam1(uint8_t _instr);

void setup()
{
  MIDI.begin();

#ifdef DO_SERIAL
  Serial.begin(9600);
  Serial.println("Debugging..");
#endif

  //pinMode(TRIGGERINPIN, INPUT);
  //triggerIn.attachCallOnRising(ISRtriggerIn);

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
  morphingInstrument.setCyclable(false);

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
  Displaywelcome();
  display.display();

  //internal clock
  Timer4.start(stepLenght);
  Timer4.attachInterrupt(ISRtriggerIn);
}

void ISRtriggerIn()
{
  Timer3.start(5000); //start 5ms triggers timers
  Timer3.attachInterrupt(endTriggers);
  tickInterval = millis() - lastTick;
  lastTick = millis();
  playMidiValue = 0;                                       //calculate final noteVelocity
  for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++) //for each instrument
  {                                                        //if it is not Actiond
    uint8_t chooseMorphedOrNot = 0;                        //force point to morph area 0
    if ((morphingInstrument.getValue() - 1) >= instr)      //if instrument already morphed
      chooseMorphedOrNot = 1;                              // point to RAM area 1
    //if not permanently muted and not momentary muted and it is a valid step
    if (!permanentMute[instr] && !instrActionState[instr] && ramPatterns[chooseMorphedOrNot][instr][stepCounter.getValue()])
    {
      playMidiValue = playMidiValue + powint(2, 5 - instr);
      digitalWrite(triggerPins[instr], HIGH);
    }
  }
  flagAdvanceStepCounter = true; //flags to point step counter to next step
}

void loop()
{
  if (playMidiValue != 0)
  {
    playMidi(1, playMidiValue, MIDICHANNEL); //send midinote with binary coded triggers message
    playMidiValue = 0;
  }

  //step counter advance ===============================================================
  //if its time to advance step counter pointer and adjust BPM if necessary
  if (flagAdvanceStepCounter)
  {
    if (flagUpdateStepLenght)
    {
      flagUpdateStepLenght = false;
      Timer4.start(stepLenght);
    }
    stepCounter.advance();
    flagAdvanceStepCounter = false;
  }

  //check for all Display updates ===============================================================

  //flags if any Display update will be necessary in this cycle
  boolean flagUpdateDisplay = false;

  //read all processed encoders interruptions
  for (uint8_t i = 0; i < MAXENCODERS; i++)
    readEncoder(i);

  //if new mood was selected
  if (flagBrowseMood)
  {
    checkDefaultDisplay();
    displayShowBrowsedMood();
    flagBrowseMood = false;
    flagUpdateDisplay = true;
  }

  //update morph bar changes
  else if (flagUpdateMorph != 0)
  {
    //if CLOCKWISE : morph increased , point actual instrumentPointers morph area [1]
    if (flagUpdateMorph == 1)
    {
      morphingInstrument.advance();
      drumKitSamplePlaying[morphingInstrument.getValue() - 1]->setValue(morphArea[1][morphingInstrument.getValue() - 1]);
      drumKitPatternPlaying[morphingInstrument.getValue() - 1]->setValue(morphArea[1][morphingInstrument.getValue() - 1 + MAXINSTRUMENTS]);
      playMidi(instrSampleMidiNote[morphingInstrument.getValue() - 1], drumKitSamplePlaying[morphingInstrument.getValue() - 1]->getValue(), MIDICHANNEL); //send new sample MIDI note do PI
    }
    //if COUNTER-CLOCKWISE : morph decreased , point actual instrumentPointers morph area [0]
    else if (flagUpdateMorph == -1)
    {
      drumKitSamplePlaying[morphingInstrument.getValue() - 1]->setValue(morphArea[0][morphingInstrument.getValue() - 1]);
      drumKitPatternPlaying[morphingInstrument.getValue() - 1]->setValue(morphArea[0][morphingInstrument.getValue() - 1 + MAXINSTRUMENTS]);
      playMidi(instrSampleMidiNote[morphingInstrument.getValue() - 1], drumKitSamplePlaying[morphingInstrument.getValue() - 1]->getValue(), MIDICHANNEL); //send new sample MIDI note do PI
      morphingInstrument.reward();                                                                                                                        //point to previous available instrument, until the first one
    }
    checkDefaultDisplay();
    displayShowMorphBar(morphingInstrument.getValue());
    flagUpdateMorph = 0;
    flagUpdateDisplay = true;
  }

  //copy selected mood to morph area
  else if (flagSelectMood)
  {
    checkDefaultDisplay();
    for (uint8_t pat = 0; pat < MAXINSTRUMENTS; pat++) // search for any BKPd pattern
      permanentMute[pat] = 0;
    displayShowPreviousMood();
    displayShowSelectedMood();

    for (uint8_t pat = 0; pat < MAXINSTRUMENTS; pat++) // search for any BKPd pattern
    {
      flagCustomPattern[pat] = 0;        //reset all custom patterns flag
      if (flagBKPdPatternFrom[pat] != 0) //if any pattern was changed , restore it from bkp before do anything
      {
        copyRomPatternToRomPattern(pat, BKPPATTERN, flagBKPdPatternFrom[pat]); //copy BKPd pattern to its original place
        flagBKPdPatternFrom[pat] = 0;
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
    displayShowMorphBar(-1);
    morphingInstrument.setValue(0);
    flagSelectMood = false;
    flagUpdateDisplay = true;
  }

  //if only one intrument pattern changed
  else if (flagUpdateThisInstrPattern != -1)
  {
    copyRomPatternToRam1(flagUpdateThisInstrPattern, flagNextRomPatternTablePointer);
    displayShowInstrumPattern(flagUpdateThisInstrPattern, RAM);
    DisplayshowSampleOrPatternNumber(false, flagNextRomPatternTablePointer);
    flagUpdateDisplay = true;
    flagUpdateThisInstrPattern = -1;
    flagNextRomPatternTablePointer = -1;
  }

  //if sampler changed
  else if (flagUpdateSampleNumber != -1)
  {
    DisplayshowSampleOrPatternNumber(true, flagUpdateSampleNumber);
    flagUpdateDisplay = true;
    flagUpdateSampleNumber = -1;
  }

  //if some pattern should be erased
  else if (flagEraseInstrumentPattern != -1)
  {
    displayBlackInstrumentPattern(flagEraseInstrumentPattern);                                                                         //clear pattern from display
    copyRomPatternToRomPattern(flagEraseInstrumentPattern, drumKitPatternPlaying[flagEraseInstrumentPattern]->getValue(), BKPPATTERN); //copy actual pattern to bkp area
    copyRomPatternToRomPattern(flagEraseInstrumentPattern, 1, drumKitPatternPlaying[flagEraseInstrumentPattern]->getValue());          //copy empty pattern to actual pattern
    // if (flagEraseInstrumentPattern < morphingInstrument.getValue())                                                                    //if instrument not morphed , force play the selected pattern
    //   drumKitPatternPlaying[flagEraseInstrumentPattern]->setValue(0);                                                                  //point actual playing to custom pattern
    clearInstrumentRam1Pattern(flagEraseInstrumentPattern); //clear actual pattern ram area
    flagUpdateDisplay = true;
    flagEraseInstrumentPattern = -1;
    flagBKPdPatternFrom[flagEraseInstrumentPattern] = flagEraseInstrumentPattern;
  }
  //if step should be tapped
  else if (flagTapInstrumentPattern != -1)
  {
    //if this pattern isnt bkp´d yet
    if (flagBKPdPatternFrom[flagTapInstrumentPattern] == 0)
    {
      copyRomPatternToRomPattern(flagTapInstrumentPattern, drumKitPatternPlaying[flagTapInstrumentPattern]->getValue(), BKPPATTERN); //copy actual pattern to bkp area
      flagBKPdPatternFrom[flagTapInstrumentPattern] = flagTapInstrumentPattern;
    }
    setStepIntoRomPattern(flagTapInstrumentPattern, drumKitPatternPlaying[flagTapInstrumentPattern]->getValue(), flagTapStep, 1); //insert new step into Rom pattern
    setStepIntoRamPattern(flagTapInstrumentPattern, flagTapStep, 1);                                                              //insert new step into Ram 1 pattern
    setStepIntoUndoArray(flagTapInstrumentPattern, flagTapStep);
    setThisPatternAsCustom(flagTapInstrumentPattern);
    displayShowInstrumPattern(flagTapInstrumentPattern, RAM); //update display with new inserted step
    flagUpdateDisplay = true;
    flagTapInstrumentPattern = -1;
    flagTapStep = -1;
  }
  else if (flagRollbackInstrumentTap != -1)
  {
    if (undoStack[0][flagRollbackInstrumentTap] != -1)
    {
      setStepIntoRomPattern(flagRollbackInstrumentTap, drumKitPatternPlaying[flagRollbackInstrumentTap]->getValue(), undoStack[0][flagRollbackInstrumentTap], 0); //insert new step into Rom pattern
      setStepIntoRamPattern(flagRollbackInstrumentTap, undoStack[0][flagRollbackInstrumentTap], 0);                                                               //insert new step into Ram 1 pattern
      rollbackStepIntoUndoArray(flagRollbackInstrumentTap);                                                                                                       //rollback the last saved step
      displayShowInstrumPattern(flagRollbackInstrumentTap, RAM);                                                                                                  //update display with new inserted step
      flagUpdateDisplay = true;
      flagRollbackInstrumentTap = -1;
    }
  }

  //if any Display update
  if (flagUpdateDisplay)
    display.display();

  //read interface inputs ===========================================================================================================
  //read all encoders buttons (invert boolean state to make easy future comparation)
  for (int8_t i = 0; i < MAXENCODERS; i++)
    encoderButtonState[i] = !digitalRead(encoderButtonPins[i]);

  //if new mood was selected....copy reference tables or create new mood in the morph area
  if (oneEncoderButtonPressed(ENCBUTMOOD) && interfaceEvent.debounced())
  {
    interfaceEvent.debounce(1000); //block any other interface event
    flagSelectMood = true;
  }

  //read all action buttons (invert boolean state to make easy future comparation)
  for (int8_t i = 0; i < MAXINSTRUMENTS; i++)
  {
    instrActionState[i] = !digitalRead(instrActionPins[i]); //read action instrument button

    //if any pattern should be erased
    //morph encoder + instrument modifier + action button + not custom pattern
    if (twoEncoderButtonsPressed(MORPHENCODER, i + 2) && instrActionState[i] && !flagCustomPattern[i] && interfaceEvent.debounced())
    {
      interfaceEvent.debounce(1000);
      flagEraseInstrumentPattern = i;
    }

    //if any tap should be rollbacked
    //morph encoder + instrument modifier + action button + custom pattern
    else if (twoEncoderButtonsPressed(MORPHENCODER, i + 2) && instrActionState[i] && flagCustomPattern[i] && interfaceEvent.debounced())
    {
      interfaceEvent.debounce(200);
      flagRollbackInstrumentTap = i;
    }

    //if any pattern should be permanently muted
    //morph encoder + action button
    else if (oneEncoderButtonPressed(MORPHENCODER) && instrActionState[i] && interfaceEvent.debounced())
    {
      permanentMute[i] = !permanentMute[i];
      interfaceEvent.debounce(1000);
    }

    //if any pattern should be tapped
    //one instrument modifier + action button
    else if (oneEncoderButtonPressed(i + 2) && instrActionState[i] && interfaceEvent.debounced())
    {
      if (millis() < (lastTick + (tickInterval >> 1))) //if we are still in the last step
      {
        stepCounter.reward();
        flagTapStep = stepCounter.getValue();
        stepCounter.advance();
        playMidi(1, powint(2, 5 - i), MIDICHANNEL); //send midinote to play only this instrument
      }
      else
      {
        flagTapStep = stepCounter.getValue();
        //no need to send midinote to play this instrument because it ill be saved to next step cycle
      }
      interfaceEvent.debounce(tickInterval);
      flagTapInstrumentPattern = i;
      //code : undo array
    }
  }
}

//close all trigger
void endTriggers()
{
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    digitalWrite(triggerPins[i], LOW);
  Timer3.stop(); //stops trigger timer
  Timer3.detachInterrupt();
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
        flagBrowseMood = true;
        lastSelectedMood = selectedMood;
      }
      break;

    case MORPHENCODER:
      //if morph button is pressed and MORPH rotate CHANGE BPM
      if (oneEncoderButtonPressed(ENCBUTMORPH))
      {
        flagUpdateStepLenght = true;
        stepLenght += -(encoderChange * 1000);
      }
      else
      { //or else , just a morph change
        //CLOCKWISE : morph increased , point actual instrumentPointers morph area [1]
        if (encoderChange == 1)
        {
          if (!morphingInstrument.isAtEnd())
            flagUpdateMorph = encoderChange;
        }
        //if COUNTER-CLOCKWISE : morph decreased , point actual instrumentPointers morph area [0]
        else if (encoderChange == -1)
        {
          if (!morphingInstrument.isAtInit())
            flagUpdateMorph = encoderChange;
        }
      }
      break;
      //all other instrument encoders
    default:
      uint8_t _instrum = _queued - 2;
      //if only one modifier pressed , change SAMPLE, schedule to change on Display too
      if (oneEncoderButtonPressed(sampleButtonModifier[_instrum]))
      {
        if (encoderChange == -1)
          drumKitSamplePlaying[_instrum]->reward();
        else
          drumKitSamplePlaying[_instrum]->advance();
        // if ((_instrum) <= (morphingInstrument.getValue()))
        morphArea[1][_instrum] = drumKitSamplePlaying[_instrum]->getValue();
        // else
        //   morphArea[0][_instrum] = drumKitSamplePlaying[_instrum]->getValue();
        playMidi(instrSampleMidiNote[_instrum], drumKitSamplePlaying[_instrum]->getValue(), MIDICHANNEL);
        flagUpdateSampleNumber = drumKitSamplePlaying[_instrum]->getValue(); //update only the sample number
      }
      else //if there is no modifier pressed , only change instrument pattern , schedule event
      {
        int8_t nextRomPatternTablePointer = morphArea[1][_instrum + MAXINSTRUMENTS]; //calculate new pattern pointer for this instrument
        nextRomPatternTablePointer += encoderChange;
        nextRomPatternTablePointer = max(1, nextRomPatternTablePointer);
        morphArea[1][_instrum + MAXINSTRUMENTS] = nextRomPatternTablePointer;                 //update destination morph area with the new pattern value
        if (_instrum < morphingInstrument.getValue())                                         //if instrument not morphed , force play the selected pattern
          drumKitPatternPlaying[_instrum]->setValue(morphArea[1][_instrum + MAXINSTRUMENTS]); //set new value
        copyRomPatternToRam1(_instrum, nextRomPatternTablePointer);                           //copy selected instrument pattern to RAM
        flagUpdateThisInstrPattern = _instrum;                                                //flags to update this instrument on Display
        flagNextRomPatternTablePointer = morphArea[1][_instrum + MAXINSTRUMENTS];             //flags to new kit value
      }
      break;
    }
  }
}

//check if default Display wasnt show yet
void checkDefaultDisplay()
{
  if (defaultDisplayNotActiveYet)
  {
    defaultDisplayNotActiveYet = false;
    display.clearDisplay();
    display.drawRect(0, 0, 60, TEXTLINE_HEIGHT - 2, WHITE);
    displayShowBrowsedMood();
    displayShowMorphBar(-1);
  }
}

void Displaywelcome()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("<-- select your mood"));
  display.println(F("    and morph it -->"));
}

void displayShowMorphBar(int8_t _size) //update morphing status / 6 steps of 10 pixels each
{
  if (lastMorphBarGraphValue != _size)
  {
    display.fillRect(1, 1, 58, TEXTLINE_HEIGHT - 4, BLACK);
    for (int8_t i = 0; i < _size; i++)
      display.fillRect(10 * i, 0, 10, TEXTLINE_HEIGHT - 2, WHITE);
    lastMorphBarGraphValue = _size;
  }
}

void DisplayshowSampleOrPatternNumber(boolean _smp, int8_t _val) //update Display right upper corner with the actual sample or pattern number
{
  display.fillRect(70, 0, Display_WIDTH - 70, TEXTLINE_HEIGHT, BLACK);
  display.setCursor(70, 0);
  display.setTextColor(WHITE);
  if (_smp)
    display.print("smp:");
  else
    display.print("pat:");
  display.print(_val);
}

void displayShowPreviousMood()
{
  display.fillRect(0, TEXTLINE_HEIGHT, Display_WIDTH, TEXTLINE_HEIGHT, BLACK);
  display.setTextColor(WHITE);
  display.setCursor(0, TEXTLINE_HEIGHT);    //set cursor position
  display.print(moodKitName[previousMood]); //print previous mood name
}
void displayShowSelectedMood()
{
  display.fillRect(0, 2 * TEXTLINE_HEIGHT, Display_WIDTH, TEXTLINE_HEIGHT, BLACK);
  display.setTextColor(WHITE);
  display.setCursor(0, 2 * TEXTLINE_HEIGHT); //set cursor position
  display.print(moodKitName[selectedMood]);  //print selected mood name
}

void displayShowBrowsedMood() //update almost all bottom Display area with the name of the selected mood and all 6 available instruments
{
  display.fillRect(0, 3 * TEXTLINE_HEIGHT, Display_WIDTH, TEXTLINE_HEIGHT, BLACK);
  display.setTextColor(WHITE);
  display.setCursor(0, 3 * TEXTLINE_HEIGHT);              //set cursor position
  display.print(moodKitName[selectedMood]);               //print mood name
  for (int8_t instr = 0; instr < MAXINSTRUMENTS; instr++) //for each instrument
    displayShowInstrumPattern(instr, ROM);
}

void displayShowInstrumPattern(uint8_t _instr, boolean _source)
{
  boolean mustPrintStep = false;
  uint8_t targetPattern = moodKitPresetPointers[selectedMood][_instr + MAXINSTRUMENTS];
  displayBlackInstrumentPattern(_instr);
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
void displayBlackInstrumentPattern(uint8_t _instr)
{
  display.fillRect(0, DOTGRIDINIT + (_instr * GRIDPATTERNHEIGHT), Display_WIDTH, GRIDPATTERNHEIGHT - 1, BLACK);
}

void playMidi(uint8_t _note, uint8_t _velocity, uint8_t _channel)
{
  MIDI.sendNoteOn(_note, _velocity, _channel);
}

//verify if ONLY ONE encoder button is pressed
boolean oneEncoderButtonPressed(uint8_t target)
{
  if (encoderButtonState[target]) //if asked button is pressed
  {
    for (int8_t i = 0; i < MAXENCODERS; i++)      //search all encoder buttons
      if ((i != target) && encoderButtonState[i]) //encoder button is not the asked one and it is pressed , so there are 2 encoder buttons pressed
        return false;
  }
  else //if not pressed , return false
    return false;
  //if all tests where OK
  return true;
}

//verify if TWO encoder button is pressed
boolean twoEncoderButtonsPressed(uint8_t target1, uint8_t target2)
{
  if (encoderButtonState[target1] && encoderButtonState[target2]) //if two asked buttons are pressed
  {
    for (int8_t i = 0; i < MAXENCODERS; i++)                         //search all encoder buttons
      if ((i != target1) && (i != target2) && encoderButtonState[i]) //encoder button is not any of asked and it is pressed , so there is a third encoder buttons pressed
        return false;
  }
  else //if not pressed , return false
    return false;
  //if all tests where OK
  return true;
}

//clear pattern from ram 1 area
void clearInstrumentRam1Pattern(uint8_t _instr)
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

void setStepIntoUndoArray(uint8_t _instr, uint8_t _step)
{
  for (uint8_t i = (MAXUNDOS - 1); i > 0; i--)
    undoStack[i][_instr] = undoStack[i - 1][_instr]; //move all values ahead to open space on stack
  undoStack[0][_instr] = _step;
}

void setThisPatternAsCustom(uint8_t _instr)
{
  flagCustomPattern[_instr] = 1;
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
