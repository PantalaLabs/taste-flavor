/*
Data flow diagram
                              +-------------+            +------------+            +---------+
                              |ref low table|            |ref hi table|            |pad table|
                              +-------------+            +------------+            +---------+
                                    |                           |                       |
                                    +---------------------------+-----------------------+
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

#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();

#include <PantalaDefines.h>
#include <EventDebounce.h>
#include <Counter.h>
#include <Rotary.h>
#include <DueTimer.h>

#define I2C_ADDRESS 0x3C
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define TEXTLINE_HEIGHT 9 // OLED display height, in pixels

//adafruit
#include <Adafruit_SSD1306.h>
#define OLED_RESET 47 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define MAXSTEPS 64      //max step sequence
#define MYCHANNEL 1      //midi channel for data transmission
#define MAXENCODERS 8    //total of encoders
#define MOODENCODER 0    //total of sample encoders
#define MORPHENCODER 1   //total of sample encoders
#define MAXINSTRUMENTS 6 //total of sample encoders
#define MIDICHANNEL 1    //standart drum midi channel
#define MOODMIDINOTE 20  //midi note to mood message
#define MORPHMIDINOTE 21 //midi note to morph message
#define BLUESCREENINIT 12
#define KICKCHANNEL 0
#define SNAKERCHANNEL 1 //snare + shaker
#define CLAPCHANNEL 2
#define HATCHANNEL 3
#define OHHRIDECHANNEL 4 //open hi hat + ride
#define PERCCHANNEL 5
#define MAXKICKPATTERNS 16
#define MAXHIPATTERNS 26
#define MAXPADPATTERNS 18

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
#define MUTEPININSTR1 6
#define MUTEPININSTR2 4
#define MUTEPININSTR3 2
#define MUTEPININSTR4 17
#define MUTEPININSTR5 31
#define MUTEPININSTR6 25
#define ENCBUTMOOD 0
#define ENCBUTMORPH 1
#define ENCBUTINSTR1 2
#define ENCBUTINSTR2 3
#define ENCBUTINSTR3 4
#define ENCBUTINSTR4 5
#define ENCBUTINSTR5 6
#define ENCBUTINSTR6 7

EventDebounce interfaceEvent(200); //min time in ms to accept another interface event

uint8_t instrSampleMidiNote[MAXINSTRUMENTS] = {10, 11, 12, 13, 14, 15};  //midi note to sample message
uint8_t instrPatternMidiNote[MAXINSTRUMENTS] = {20, 21, 22, 23, 24, 25}; //midi note to pattern message
uint8_t triggerPins[MAXINSTRUMENTS] = {TRIGOUTPIN1, TRIGOUTPIN2, TRIGOUTPIN3, TRIGOUTPIN4, TRIGOUTPIN5, TRIGOUTPIN6};
//encoders buttons : mood, morph, instruments
uint8_t encoderButtonPins[MAXENCODERS] = {ENCBUTPINMOOD, ENCBUTPINMORPH, ENCBUTPININSTR1, ENCBUTPININSTR2, ENCBUTPININSTR3, ENCBUTPININSTR4, ENCBUTPININSTR5, ENCBUTPININSTR6};
uint8_t sampleButtonModifier[MAXINSTRUMENTS] = {ENCBUTINSTR2, ENCBUTINSTR1, ENCBUTINSTR4, ENCBUTINSTR3, ENCBUTINSTR6, ENCBUTINSTR5}; //this points to each instrument sample change button modifier

boolean encoderButtonState[MAXENCODERS] = {0, 0, 0, 0, 0, 0, 0, 0};

//mute buttons
uint8_t instrMutePins[MAXINSTRUMENTS] = {MUTEPININSTR1, MUTEPININSTR2, MUTEPININSTR3, MUTEPININSTR4, MUTEPININSTR5, MUTEPININSTR6}; //pins
boolean instrMuteState[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};

//playing instrument pattern pointer
Counter instr1patternPointer(MAXKICKPATTERNS - 1);
Counter instr2patternPointer(MAXHIPATTERNS - 1);
Counter instr3patternPointer(MAXHIPATTERNS - 1);
Counter instr4patternPointer(MAXHIPATTERNS - 1);
Counter instr5patternPointer(MAXHIPATTERNS - 1);
Counter instr6patternPointer(MAXPADPATTERNS - 1);
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

uint16_t selectedMood = 0;
uint8_t lastSelectedMood = 255;      //prevents to execute 2 times the same action
int8_t lastMorphBarGraphValue = 127; //0 to MAXINSTRUMENTS possible values
boolean flagBrowseMood = false;      //schedule some screen update
int8_t flagUpdateMorph = 0;          //schedule some screen update
boolean flagSelectMood = false;      //schedule some screen update
int8_t flagUpdateThisInstrPattern = -1;
int8_t flagUpdateThisMorphedDrumKit = -1;
int8_t flagUpdateSampleNumber = -1;
int8_t flagEraseInstrumentPattern = -1;
int8_t flagTapInstrumentPattern = -1;
int8_t flagTapStep = -1;
boolean defaultScreenNotActiveYet = true;

boolean romPatternTableLow[MAXKICKPATTERNS][MAXSTEPS] = {
    //, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _},
    //, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //..
    {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0}, //4x4
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}, //4x4 + 1
    {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}, //4x4 + 1
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //tum tum...
    {1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //
    {0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0}, //
    {1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0},
    {1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

};

boolean romPatternTableHi[MAXHIPATTERNS][MAXSTEPS] = {
    //, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _},
    //, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //..
    {0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0}, //tss
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0}, //tah
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}, //tss......ts...
    {1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}, //ti di di ....ti .....ti
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0}, //......tei tei tei
    {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0}, //tah tah ath ath
    {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, //tah tah ath ath
    {1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0}, //ti di di rápido
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, //tststststststst
    {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, .0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0},

};

boolean romPatternTablePad[MAXPADPATTERNS][MAXSTEPS] = {
    //, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _},
    //, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //..
    {1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0},
    {1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0},
    {1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1},
    {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0},
    {1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0},
    {1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1},
    {1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0},
    {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1},
    {1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0},
    {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0}};

//MOOD===================================================================================================================
//{name , action} action 0=mood switch / 1=mood command
#define MAXMOODS 5 //max moods
int16_t moodDb[2][MAXMOODS] = {{-1, 3, 1, 0, 2},
                               //4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
                               {3, 2, 4, 1, 5}};
// 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25}};
String moodName[MAXMOODS] = {"", "4x4 mood 1", "4x4 mood 2", "4x4 mood 3", "4x4 mood 4"};

// {"4x4 mood 5"},
// {"4x4 mood 6"},
// {"Mood9"},
// {"Mood10"},
// {"Mood11"},
// {"Mood12"},
// {"Mood13"},
// {"mood14"},
// {"Import 1"},
// {"Import 1"},
// {"Import 2"},
// {"Import 3"},
// {"Import 4"},
// {"Import 1"},
// {"Import 2"},
// {"Import 3"},
// {"Add more Hats 1"}
// {"Lofi bass"}},
// {"Lofi hat"}};

//SAMPLES + PATTERNS + PREVIOUS + NEXT
int8_t moodKitPresetPointers[MAXMOODS][(2 * MAXINSTRUMENTS) + 2] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 1},
    {6, 4, 5, 3, 3, 0, 7, 3, 5, 4, 3, 0, 0, 2},
    {11, 4, 15, 3, 13, 3, 3, 3, 6, 1, 3, 0, 1, 3},
    {11, 5, 15, 3, 18, 2, 3, 3, 6, 1, 0, 0, 2, 4},
    {15, 5, 15, 3, 18, 1, 3, 3, 6, 0, 4, 2, 3, 5}};

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
uint8_t morphArea[2][2 * MAXINSTRUMENTS] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
boolean ramPatterns[2][6][MAXSTEPS];

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
void checkDefaultScreen();
void SCREENwelcome();
void SCREENvisualizeMood();
void SCREENupdateMorphBar(int8_t _size);
void SCREENupdateSampleOrPatternNumber(boolean _smp, int8_t _val);
void SCREENprintStepDot(uint8_t _step, uint8_t _instr);
void SCREENeraseInstrumentPattern(uint8_t _instr);
void SCREENprintInstrumPattern(uint8_t _instr, boolean _source);
void endTriggers();
boolean onlyOneEncoderButtonTrue(uint8_t target);
boolean onlyTwoEncoderButtonTrue(uint8_t target1, uint8_t target2);
void copyRomPatternToRam1(uint8_t _instr, uint8_t _romPatternTablePointer);
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

  //start all mute buttons
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    pinMode(instrMutePins[i], INPUT_PULLUP);

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
  SCREENwelcome();
  display.display();

  //internal clock
  Timer4.start(stepLenght);
  Timer4.attachInterrupt(ISRtriggerIn);
}

void ISRtriggerIn()
{
  Timer3.start(5000); //start 5ms triggers timers
  Timer3.attachInterrupt(endTriggers);
  playMidiValue = 0;                                                                              //calculate final noteVelocity
  for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++)                                        //for each instrument
  {                                                                                               //if it is not muted
    uint8_t chooseMorphedOrNot = 0;                                                               //force point to morph area 0
    if ((morphingInstrument.getValue() - 1) >= instr)                                             //if instrument already morphed
      chooseMorphedOrNot = 1;                                                                     // point to RAM area 1
    if (!instrMuteState[instr] && ramPatterns[chooseMorphedOrNot][instr][stepCounter.getValue()]) //if not muted and valid step
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

  //check for all screen updates ===============================================================

  //flags if any screen update will be necessary in this cycle
  boolean flagUpdateScreen = false;

  //read all processed encoders interruptions
  for (uint8_t i = 0; i < MAXENCODERS; i++)
    readEncoder(i);

  //if new mood was selected
  if (flagBrowseMood)
  {
    checkDefaultScreen();
    SCREENvisualizeMood();
    flagBrowseMood = false;
    flagUpdateScreen = true;
  }

  //update morph bar changes
  if (flagUpdateMorph != 0)
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
    checkDefaultScreen();
    SCREENupdateMorphBar(morphingInstrument.getValue());
    flagUpdateMorph = 0;
    flagUpdateScreen = true;
  }

  //copy selected mood to morph area
  if (flagSelectMood)
  {
    checkDefaultScreen();
    //copy selected mood samples and patterns pointers to or from morph area
    for (uint8_t i = 0; i < MAXINSTRUMENTS; i++) //for each instrument
    {

      //update morph area 0
      morphArea[0][i] = drumKitSamplePlaying[i]->getValue();                   //copy playing sample to morphArea 0
      morphArea[0][i + MAXINSTRUMENTS] = drumKitPatternPlaying[i]->getValue(); //copy playing pattern to morphArea 0
      copyRamPattern1ToRam0(i);                                                //copy ramPattern area 1 to ramPattern area 0
      //update morph area 1
      if (moodKitPresetPointers[selectedMood][i] != -1)                  //if selected mood sample set NOT to REUSE
        morphArea[1][i] = moodKitPresetPointers[selectedMood][i];        //copy new selected mood sample to morphArea 1
      else                                                               //else
        morphArea[1][i] = morphArea[0][i];                               //reuse sample
      if (moodKitPresetPointers[selectedMood][i + MAXINSTRUMENTS] != -1) //if avalilable instrument pattern
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
    SCREENupdateMorphBar(-1);
    morphingInstrument.setValue(0);
    flagSelectMood = false;
    flagUpdateScreen = true;
  }

  //if only one intrument pattern changed
  if (flagUpdateThisInstrPattern != -1)
  {
    SCREENeraseInstrumentPattern(flagUpdateThisInstrPattern);
    copyRomPatternToRam1(flagUpdateThisInstrPattern, flagUpdateThisMorphedDrumKit);
    SCREENprintInstrumPattern(flagUpdateThisInstrPattern, 0);
    SCREENupdateSampleOrPatternNumber(false, flagUpdateThisMorphedDrumKit);
    flagUpdateScreen = true;
    flagUpdateThisInstrPattern = -1;
    flagUpdateThisMorphedDrumKit = -1;
  }

  //if sampler changed
  if (flagUpdateSampleNumber != -1)
  {
    SCREENupdateSampleOrPatternNumber(true, flagUpdateSampleNumber);
    flagUpdateScreen = true;
    flagUpdateSampleNumber = -1;
  }

  //if new mood was selected....copy reference tables or create new mood in the morph area
  if (onlyOneEncoderButtonTrue(ENCBUTMOOD) && interfaceEvent.debounced())
  {
    interfaceEvent.debounce(); //block any other interface event
    flagSelectMood = true;
  }

  //if some pattern should be erased
  if (flagEraseInstrumentPattern != -1)
  {
    SCREENeraseInstrumentPattern(flagEraseInstrumentPattern);
    morphArea[1][flagEraseInstrumentPattern + MAXINSTRUMENTS] = 0;
    if (flagEraseInstrumentPattern < morphingInstrument.getValue())   //if instrument not morphed , force play the selected pattern
      drumKitPatternPlaying[flagEraseInstrumentPattern]->setValue(0); //set new value
    for (int8_t step = 0; step < MAXSTEPS; step++)
      ramPatterns[1][flagEraseInstrumentPattern][step] = 0;
    flagUpdateScreen = true;
    flagEraseInstrumentPattern = -1;
  }

  //if some step should be tapped
  // if (flagTapInstrumentPattern != -1)
  // {
  //   //SCREENeraseInstrumentPattern(flagEraseInstrumentPattern);
  //   if (flagEraseInstrumentPattern < morphingInstrument.getValue())   //if instrument not morphed , force play the selected pattern
  //     drumKitPatternPlaying[flagEraseInstrumentPattern]->setValue(0); //set new value
  //   flagUpdateScreen = true;
  //   flagTapInstrumentPattern = -1;
  //   flagTapStep = -1;
  // }

  //read interface inputs ===============================================================
  //if any screen update
  if (flagUpdateScreen)
    display.display();

  //read all encoders buttons (invert boolean state to make easy future comparation)
  for (int8_t i = 0; i < MAXENCODERS; i++)
    encoderButtonState[i] = !digitalRead(encoderButtonPins[i]);

  //read if any instrument should be muted (invert boolean state to make easy future comparation)
  for (int8_t i = 0; i < MAXINSTRUMENTS; i++)
  {
    instrMuteState[i] = !digitalRead(instrMutePins[i]);
    //check if any pattern should be erased
    if (instrMuteState[i] && onlyOneEncoderButtonTrue(MORPHENCODER))
      flagEraseInstrumentPattern = i;
    //check if any pattern should be tapped
    if (instrMuteState[i] && onlyOneEncoderButtonTrue(i + 2))
    {
      flagTapInstrumentPattern = i;
      flagTapStep = stepCounter.getValue();
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
      if (encoderChange == 1)
      { //point to next mood on Db
        if (moodDb[1][selectedMood] != MAXMOODS)
          selectedMood = moodDb[1][selectedMood];
      }
      else
      { //point to previous mood on Db
        if (moodDb[0][selectedMood] != -1)
          selectedMood = moodDb[0][selectedMood];
      }
      //saves CPU if mood changed but still in the same position , in the start and in the end of the list
      if (lastSelectedMood != selectedMood)
      {
        flagBrowseMood = true;
        lastSelectedMood = selectedMood;
      }
      break;

    case MORPHENCODER:
      //if morph button is pressed and MORPH rotate CHANGE BPM
      if (onlyOneEncoderButtonTrue(ENCBUTMORPH))
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
      uint8_t realEncoder = _queued - 2;
      //if only one modifier pressed , change SAMPLE, schedule to change on screen too
      if (onlyOneEncoderButtonTrue(sampleButtonModifier[realEncoder]))
      {
        if (encoderChange == -1)
          drumKitSamplePlaying[realEncoder]->reward();
        else
          drumKitSamplePlaying[realEncoder]->advance();
        // if ((realEncoder) <= (morphingInstrument.getValue()))
        morphArea[1][realEncoder] = drumKitSamplePlaying[realEncoder]->getValue();
        // else
        //   morphArea[0][realEncoder] = drumKitSamplePlaying[realEncoder]->getValue();
        playMidi(instrSampleMidiNote[realEncoder], drumKitSamplePlaying[realEncoder]->getValue(), MIDICHANNEL);
        flagUpdateSampleNumber = drumKitSamplePlaying[realEncoder]->getValue(); //update only the sample number
        //Serial.println(flagUpdateSampleNumber);
      }
      else //if there is no modifier pressed , CHANGE PATTERN, schedule to change on screen too
      {
        //change TARGET PATTERN================================
        int8_t nextRomPatternTablePointer = morphArea[1][realEncoder + MAXINSTRUMENTS];
        nextRomPatternTablePointer += encoderChange;
        nextRomPatternTablePointer = max(0, nextRomPatternTablePointer);
        morphArea[1][realEncoder + MAXINSTRUMENTS] = nextRomPatternTablePointer;                    //update destination morph area with the new pattern value
        if (realEncoder < morphingInstrument.getValue())                                            //if instrument not morphed , force play the selected pattern
          drumKitPatternPlaying[realEncoder]->setValue(morphArea[1][realEncoder + MAXINSTRUMENTS]); //set new value
        copyRomPatternToRam1(realEncoder, nextRomPatternTablePointer);                              //copy selected instrument pattern to RAM
        flagUpdateThisInstrPattern = realEncoder;                                                   //flags to update this instrument on screen
        flagUpdateThisMorphedDrumKit = morphArea[1][realEncoder + MAXINSTRUMENTS];                  //flags to new kit value
      }
      break;
    }
    //if DOUBLE modifier pressed , CHANGE anything else
    // if (encoderButtonState[ENCBUTINSTR1] && encoderButtonState[ENCBUTINSTR2])
    // {
    // }
  }
}

//check if default screen wasnt show yet
void checkDefaultScreen()
{
  if (defaultScreenNotActiveYet)
  {
    defaultScreenNotActiveYet = false;
    display.clearDisplay();
    display.drawRect(0, 0, 60, TEXTLINE_HEIGHT - 2, WHITE);
    SCREENvisualizeMood();
    SCREENupdateMorphBar(-1);
  }
}

void SCREENwelcome()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("<-- select your mood"));
  display.println(F("    and morph it -->"));
}

//erase exatctly one line pattern
void SCREENeraseInstrumentPattern(uint8_t _instr)
{
  display.fillRect(0, BLUESCREENINIT + 5 + (_instr * 7), SCREEN_WIDTH, 6, BLACK);
}

//printn one step dot
void SCREENprintStepDot(uint8_t _step, uint8_t _instr)
{
  uint8_t validStep;
  validStep = _step;
  display.fillRect(validStep * 2, BLUESCREENINIT + 5 + (_instr * 7), 1, 5, WHITE);

  // display.setCursor(_step * 2, BLUESCREENINIT + (_instr * 3));
  // display.print(F("."));
}

//update morphing status / 6 steps of 10 pixels each
void SCREENupdateMorphBar(int8_t _size)
{
  if (lastMorphBarGraphValue != _size)
  {
    display.fillRect(1, 1, 58, TEXTLINE_HEIGHT - 4, BLACK);
    for (int8_t i = 0; i < _size; i++)
      display.fillRect(10 * i, 0, 10, TEXTLINE_HEIGHT - 2, WHITE);
    lastMorphBarGraphValue = _size;
  }
}

//update screen right upper corner with the actual sample or pattern number
void SCREENupdateSampleOrPatternNumber(boolean _smp, int8_t _val)
{
  display.fillRect(70, 0, SCREEN_WIDTH - 70, TEXTLINE_HEIGHT, BLACK);
  display.setCursor(70, 0);
  display.setTextColor(WHITE);
  if (_smp)
  {
    display.print("smp:");
  }
  else
  {
    display.print("pat:");
  }
  display.print(_val);
}

//update almost all bottom screen area with the name of the selected mood and all 6 available instruments
void SCREENvisualizeMood()
{
  display.setTextColor(WHITE);
  display.fillRect(0, TEXTLINE_HEIGHT, SCREEN_WIDTH, TEXTLINE_HEIGHT, BLACK);
  display.setCursor(0, TEXTLINE_HEIGHT);                  //set cursor position
  display.print(moodName[selectedMood]);                  //print mood name
  for (int8_t instr = 0; instr < MAXINSTRUMENTS; instr++) //for each instrument
  {
    SCREENeraseInstrumentPattern(instr);
    SCREENprintInstrumPattern(instr, 1);
    // for (int8_t step = 0; step < (MAXSTEPS - 2); step++) //for each step
    // {
    //   // if (moodKitPresetPointers[selectedMood][instr + MAXINSTRUMENTS] > 0) //skip if pattern is null or nothing
    // {
    //   boolean mustPrintStep = false;
    //   if (selectedMood == KICKCHANNEL)
    //   {
    //     if (romPatternTableLow[moodKitPresetPointers[selectedMood][instr + MAXINSTRUMENTS]][step] == 1) //if it is an active step
    //       mustPrintStep = true;
    //   }
    //   else if ((selectedMood == SNAKERCHANNEL) ||
    //            (selectedMood == CLAPCHANNEL) ||
    //            (selectedMood == HATCHANNEL) ||
    //            (selectedMood == OHHRIDECHANNEL))
    //   {
    //     if (romPatternTableHi[moodKitPresetPointers[selectedMood][instr + MAXINSTRUMENTS]][step] == 1) //if it is an active step
    //       mustPrintStep = true;
    //   }
    //   else
    //   {
    //     //pad table
    //     if (romPatternTablePad[moodKitPresetPointers[selectedMood][instr + MAXINSTRUMENTS]][step] == 1) //if it is an active step
    //       mustPrintStep = true;
    //   }
    //   if (mustPrintStep)
    //     SCREENprintStepDot(step, instr);
  }
}

void SCREENprintInstrumPattern(uint8_t _instr, boolean _source)
{
  display.setTextColor(WHITE);
  for (int8_t step = 0; step < (MAXSTEPS - 2); step++) //for each step
  {
    if ((_source == 0) && (ramPatterns[1][_instr][step])) //if source = RAM
      mustPrintStep = true;
    else
    {                                                                       //source = ROM
      if (moodKitPresetPointers[selectedMood][_instr + MAXINSTRUMENTS] > 0) //skip if pattern is null or nothing
      {
        boolean mustPrintStep = false;
        if (selectedMood == KICKCHANNEL)
        {
          if (romPatternTableLow[moodKitPresetPointers[selectedMood][_instr + MAXINSTRUMENTS]][step] == 1) //if it is an active step
            mustPrintStep = true;
        }
        else if ((selectedMood == SNAKERCHANNEL) ||
                 (selectedMood == CLAPCHANNEL) ||
                 (selectedMood == HATCHANNEL) ||
                 (selectedMood == OHHRIDECHANNEL))
        {
          if (romPatternTableHi[moodKitPresetPointers[selectedMood][_instr + MAXINSTRUMENTS]][step] == 1) //if it is an active step
            mustPrintStep = true;
        }
        else
        {
          //pad table
          if (romPatternTablePad[moodKitPresetPointers[selectedMood][_instr + MAXINSTRUMENTS]][step] == 1) //if it is an active step
            mustPrintStep = true;
        }
      }
    }
    if (mustPrintStep)
      SCREENprintStepDot(step, _instr);
  }
}

void playMidi(uint8_t _note, uint8_t _velocity, uint8_t _channel)
{
  MIDI.sendNoteOn(_note, _velocity, _channel);
}

//verify if ONLY ONE encoder button is pressed
boolean onlyOneEncoderButtonTrue(uint8_t target)
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
boolean onlyTwoEncoderButtonTrue(uint8_t target1, uint8_t target2)
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

void copyRomPatternToRam1(uint8_t _instr, uint8_t _romPatternTablePointer) //copy selected instrument pattern from ROM to RAM
{
  for (uint8_t i = 0; i < MAXSTEPS; i++)
  {
    if (_instr == KICKCHANNEL)
    {
      ramPatterns[1][_instr][i] = romPatternTableLow[_romPatternTablePointer][i];
    }
    else if ((_instr == SNAKERCHANNEL) ||
             (_instr == CLAPCHANNEL) ||
             (_instr == HATCHANNEL) ||
             (_instr == OHHRIDECHANNEL))
    {
      ramPatterns[1][_instr][i] = romPatternTableHi[_romPatternTablePointer][i];
    }
    else
    {
      ramPatterns[1][_instr][i] = romPatternTablePad[_romPatternTablePointer][i];
    }
  }
}

void copyRamPattern1ToRam0(uint8_t _instr) //copy ramPattern area 1 to ramPattern area 0
{
  for (uint8_t i = 0; i < MAXSTEPS; i++)
    ramPatterns[0][_instr][i] = ramPatterns[1][_instr][i];
}

void copyRamPattern0ToRam1(uint8_t _instr) //copy ramPattern area 0 to ramPattern area 1
{
  for (uint8_t i = 0; i < MAXSTEPS; i++)
    ramPatterns[1][_instr][i] = ramPatterns[0][_instr][i];
}