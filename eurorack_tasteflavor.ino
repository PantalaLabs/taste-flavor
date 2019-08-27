/*
Data flow diagram
                              +-------------+            +------------+            +---------+
                              |ref low table|            |ref hi table|            |pad table|
                              +-------------+            +------------+            +---------+
                                    |                           |                       |
                                    +---------------------------+-----------------------+
                                    |
                                    V
                           +--------------------+            +-------------------+          +-----------------------------+
                           |instrXPatternPointer|            |instrXsamplePointer| <--------|rpi available sample quantity|
                           +--------------------+            +-------------------+          +-----------------------------+
                                       |                                |
                                       +--------------------------------+
                                                      |
                                                      V
                                                actual playing instr.
                        morphPattern <------>   drumKitPatternPlaying  <------> encoderPatternChange
                        morphSample  <------>   drumKitSamplePlaying   <------> encoderSampleChange

*/

#define DOMIDI true
#define OPENSERIAL false
#define OPENMIDIUSB false

#include <PantalaDefines.h>
#include <Switch.h>
#include <MicroDebounce.h>
#include <EventDebounce.h>
#include <Counter.h>
#include <Trigger.h>
#include <Rotary.h>
#include <MIDIUSB.h>
//#include <DueTimer.h>

#ifdef DOMIDI
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();
#endif

#define I2C_ADDRESS 0x3C
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define TEXTLINE_HEIGHT 9 // OLED display height, in pixels

//adafruit
#include <Adafruit_SSD1306.h>
#define OLED_RESET 4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define MAXSTEPS 64      //max step sequence
#define MYCHANNEL 1      //midi channel for data transmission
#define MAXENCODERS 8    //total of encoders
#define MOODENCODER 0    //total of sample encoders
#define MORPHENCODER 1   //total of sample encoders
#define MAXINSTRUMENTS 6 //total of sample encoders
#define TRIGGERINPIN 7   //total of sample encoders
#define MIDICHANNEL 1    //standart drum midi channel
#define MOODMIDINOTE 20  //midi note to mood message
#define MORPHMIDINOTE 21 //midi note to morph message
#define patternScreenYinit 12
#define KICKCHANNEL 0
#define SNAKERCHANNEL 1 //snare + shaker
#define CLAPCHANNEL 2
#define HATCHANNEL 3
#define OHHRIDECHANNEL 4 //open hi hat + ride
#define PERCCHANNEL 5
#define MAXKICKPATTERNS 16
#define MAXHIPATTERNS 26
#define MAXPADPATTERNS 18

uint8_t instrSampleMidiNote[MAXINSTRUMENTS] = {10, 11, 12, 13, 14, 15};  //midi note to sample message
uint8_t instrPatternMidiNote[MAXINSTRUMENTS] = {20, 21, 22, 23, 24, 25}; //midi note to pattern message
EventDebounce interfaceEvent(200);                                       //min time in ms to accept another interface event
MicroDebounce stepLenght(125000);                                        //step sequence interval
MicroDebounce freeZone1(4000);                                           //first 4ms available toprocess light interface events
MicroDebounce triggerZone(2000);                                         //freeze cpu 2ms to close the triggers
MicroDebounce freeZone2(70000);                                          //big cpu time available to process screen and other stuff
#define L2CSAFETY 35000UL                                                //48ms L2C communication on oscilloscope =  38000UL
/*
      +-----+                                             
      |     |                                             
______|     |____________________________________________|
      |<-->|freeZone1                     
          >|-|< triggerZone
             |<---------freeZone2------->|
                                         |<--L2CSAFETY-->|
      |<--------------------stepLenght------------------>|
*/

//trigger out pins
Trigger instr1trigger(5, true);
Trigger instr2trigger(3, true);
Trigger instr3trigger(16, true);
Trigger instr4trigger(18, true);
Trigger instr5trigger(23, true);
Trigger instr6trigger(25, true);
Trigger *triggers[MAXINSTRUMENTS] = {&instr1trigger, &instr2trigger, &instr3trigger, &instr4trigger, &instr5trigger, &instr6trigger};
// uint8_t triggerPins[MAXINSTRUMENTS] = {5, 3, 16, 18, 23, 25};
// boolean triggerBus[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};

//encoders buttons
Switch encoderButtonMood(11, true);
Switch encoderButtonMorph(8, true);
Switch encoderButtonChannel1(52, true);
Switch encoderButtonChannel2(34, true);
Switch encoderButtonChannel3(46, true);
Switch encoderButtonChannel4(28, true);
Switch encoderButtonChannel5(40, true);
Switch encoderButtonChannel6(22, true);
Switch *encoderButtons[MAXENCODERS] = {&encoderButtonMood, &encoderButtonMorph, &encoderButtonChannel1, &encoderButtonChannel2, &encoderButtonChannel3, &encoderButtonChannel4, &encoderButtonChannel5, &encoderButtonChannel6};
Counter encoderButtonQueue(MAXENCODERS - 1);

//mute buttons
Switch instr1mute(6, true);
Switch instr2mute(4, true);
Switch instr3mute(2, true);
Switch instr4mute(17, true);
Switch instr5mute(19, true);
Switch instr6mute(27, true);
Switch *instrumentMute[MAXINSTRUMENTS] = {&instr1mute, &instr2mute, &instr3mute, &instr4mute, &instr5mute, &instr6mute};
Counter instrumentMuteQueue(MAXINSTRUMENTS - 1);

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

Rotary MOODencoder(13, 12);
Rotary MORPHencoder(10, 9);
Rotary instr1encoder(50, 48);
Rotary instr2encoder(32, 30);
Rotary instr3encoder(44, 42);
Rotary instr4encoder(26, 24);
Rotary instr5encoder(38, 36);
Rotary instr6encoder(14, 15);
Rotary *encoders[MAXENCODERS] = {&MOODencoder, &MORPHencoder, &instr1encoder, &instr2encoder, &instr3encoder, &instr4encoder, &instr5encoder, &instr6encoder};

//Switch triggerIn(TRIGGERINPIN);
Counter stepCounter(MAXSTEPS - 1);
Counter encoderQueue(MAXENCODERS - 1);

boolean playSteps = true;

uint16_t moodPointer = 0;
Counter morphingInstrument(MAXINSTRUMENTS);
uint8_t lastSelectedMood = 255;          //prevents to execute 2 times the same action
uint8_t lastSelectedMoodSelection = 255; //prevents to execute 2 times the same action
int8_t lastMorphBarGraphValue = 127;     //0 to MAXINSTRUMENTS possible values
uint32_t lastTick;                       //last time step counter was ticked
uint32_t tickInterval;                   //time in us between two ticks
boolean flagUpdateMood = false;          //schedule some screen update
int8_t flagUpdateMorph = 0;              //schedule some screen update
boolean flagSelectMood = false;          //schedule some screen update
int8_t flagUpdateThisMorphedPattern = -1;
int8_t flagUpdateThisMorphedDrumKit = -1;
int8_t flagUpdateSampleNumber = -1;

boolean defaultScreenNotActiveYet = true;

boolean referencePatternTableLow[MAXKICKPATTERNS][MAXSTEPS] = {
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

boolean referencePatternTableHi[MAXHIPATTERNS][MAXSTEPS] = {
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

boolean referencePatternTablePad[MAXPADPATTERNS][MAXSTEPS] = {
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
int8_t drumKit[MAXMOODS][(2 * MAXINSTRUMENTS) + 2] = {
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

//MORPH 6 samples + 6 patterns
uint8_t morphArea[2][2 * MAXINSTRUMENTS] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
// #define MAXLOFIKICK 11
// uint8_t lofiKick[MAXLOFIKICK] = {0, 3, 4, 12, 17, 23, 25, 42, 43, 44, 45};
// #define MAXLOFIHAT 10
// uint8_t lofiHihat[MAXLOFIHAT] = {6, 16, 17, 22, 24, 33, 42, 50, 53, 62};

void ISRtriggerIn();
void playMidi(uint8_t _note, uint8_t _velocity, uint8_t _channel);
void SCREENwelcome();
void SCREENdefault();
void SCREENupdateMood();
void SCREENprintStepDot(uint8_t _mood, uint8_t _i, uint8_t _j);
void SCREENupdateMorphBar(int8_t _size);
void readEncoder();
void checkDefaultScreen();
void startTriggerZone();
void startFreeZone2();
void SCREENupdateSampleAndPatternNumber(int8_t _val);
void endTriggers();

void ISRtriggerIn()
{
  //playSteps = !playSteps;
}

void startTriggerZone()
{
  triggerZone.debounce();
}

void startFreeZone2()
{
  uint32_t newInterval;
  newInterval = (uint32_t)lastTick + (uint32_t)tickInterval - ((uint32_t)micros() + (uint32_t)L2CSAFETY);
  freeZone2.debounce(newInterval);
}

void setup()
{
  freeZone1.attachCallOnDebounce(startTriggerZone); //freeze CPU to close the triggers
  triggerZone.attachCallOnDebounce(startFreeZone2); //calculate available cpu right afetr the triggers got closed
  if (OPENSERIAL)
  {
    Serial.begin(9600);
    Serial.println("Debugging..");
  }

#ifdef DOMIDI
  MIDI.begin();
#endif

#ifdef OPENMIDIUSB
  Serial.begin(115200);
#endif

  //triggerIn.attachCallOnRising(ISRtriggerIn);
  for (uint8_t i = 0; i < MAXENCODERS; i++)
  {
    encoders[i]->begin();
    delay(10);
  }

  //set these counters to not cyclabe
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
  {
    drumKitPatternPlaying[i]->setCyclable(false);
    drumKitSamplePlaying[i]->setCyclable(false);
  }
  morphingInstrument.setCyclable(false);

  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS))
  { // Address 0x3D for 128x64
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
}

void loop()
{
  //PRIORITY 0 - play next step
  if (playSteps && stepLenght.debounced())
  {
    // Timer3.attachInterrupt(endTriggers);
    // Timer3.start(5000);
    tickInterval = micros() - lastTick; //total pulse interval
    lastTick = micros();                //save this tick time
    stepLenght.debounce();              //start new step interval
    freeZone1.debounce();               //frtees cpu to do small things
    uint8_t noteVelocity = 0;           //calculate final noteVelocity
    for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    {
      //if instrument not muted
      if (
          instrumentMute[i]->active() &&                                                                                             //
          (((i == 0) && (referencePatternTableLow[drumKitPatternPlaying[i]->getValue()][stepCounter.getValue()] == 1)) ||            //
           (((i > 0) && (i < 5)) && (referencePatternTableHi[drumKitPatternPlaying[i]->getValue()][stepCounter.getValue()] == 1)) || //
           ((i == 5) && (referencePatternTablePad[drumKitPatternPlaying[i]->getValue()][stepCounter.getValue()] == 1))               //
           )                                                                                                                         //
      )
      {
        noteVelocity = noteVelocity + powint(2, 5 - i);
        triggers[i]->start();
      }
    }
    //send midinote with binary coded triggers message
    if (noteVelocity != 0)
      playMidi(1, noteVelocity, MIDICHANNEL);
    stepCounter.advance();
  }

  //if cpu available for large display updates
  boolean flagUpdateScreen = false;
  if (!freeZone2.debounced())
  {
    //update mood changes
    if (flagUpdateMood)
    {
      checkDefaultScreen();
      SCREENupdateMood();
      flagUpdateMood = false;
      flagUpdateScreen = true;
    }

    //update morph bar changes
    if (flagUpdateMorph != 0)
    {
      if (flagUpdateMorph == 1)
      {
        morphingInstrument.advance();
        drumKitSamplePlaying[morphingInstrument.getValue() - 1]->setValue(morphArea[1][morphingInstrument.getValue() - 1]);
        drumKitPatternPlaying[morphingInstrument.getValue() - 1]->setValue(morphArea[1][morphingInstrument.getValue() - 1 + MAXINSTRUMENTS]);
        playMidi(instrSampleMidiNote[morphingInstrument.getValue() - 1], drumKitSamplePlaying[morphingInstrument.getValue() - 1]->getValue(), MIDICHANNEL);
      }
      //if COUNTER-CLOCKWISE : morph decreased , point actual instrumentPointers morph area [0]
      else if (flagUpdateMorph == -1)
      {
        drumKitSamplePlaying[morphingInstrument.getValue() - 1]->setValue(morphArea[0][morphingInstrument.getValue() - 1]);
        drumKitPatternPlaying[morphingInstrument.getValue() - 1]->setValue(morphArea[0][morphingInstrument.getValue() - 1 + MAXINSTRUMENTS]);
        playMidi(instrSampleMidiNote[morphingInstrument.getValue() - 1], drumKitSamplePlaying[morphingInstrument.getValue() - 1]->getValue(), MIDICHANNEL);
        morphingInstrument.reward(); //point to previous available instrument, until the first one
      }
      checkDefaultScreen();
      SCREENupdateMorphBar(morphingInstrument.getValue());
      flagUpdateMorph = 0;
      flagUpdateScreen = true;
    }

    //copy selected mood to morph area
    if (flagSelectMood)
    {
      if (lastSelectedMoodSelection != moodPointer)
      {
        checkDefaultScreen();
        // case '0':
        for (uint8_t i = 0; i < MAXINSTRUMENTS; i++) //copy selected mood tables to morph area
        {
          //copies the ALL ACTUAL PLAYING SAMPLE AND PATTERN to morph area 0
          morphArea[0][i] = drumKitSamplePlaying[i]->getValue();                   //copy playing sample to morphArea 0
          morphArea[0][i + MAXINSTRUMENTS] = drumKitPatternPlaying[i]->getValue(); //copy playing pattern to morphArea 0
          //copy the new user selected sample to morph area 1
          if (drumKit[moodPointer][i] != -1)           //if selected mood sample not an mood "add more"
            morphArea[1][i] = drumKit[moodPointer][i]; //copy selected mood sample to morphArea 1
          else                                         //else
            morphArea[1][i] = morphArea[0][i];         //use actual playing sample
          //copies the selected instrument to morph area 1
          if (drumKit[moodPointer][i + MAXINSTRUMENTS] != -1)                            //if selected mood pattern not a mood "add more"
            morphArea[1][i + MAXINSTRUMENTS] = drumKit[moodPointer][i + MAXINSTRUMENTS]; //copy selected mood pattern to morphArea 1
          else                                                                           //else
            morphArea[1][i + MAXINSTRUMENTS] = morphArea[0][i + MAXINSTRUMENTS];         //use actual playing pattern
        }
        //break;
        // // behavior command request
        // case 1: //lo fi kick
        //   randomSeed(millis());

        //   morphSample[0][0] = lofiKick[random(MAXLOFIKICK - 1)];            //use new lofi selected sample
        //   morphSample[1][0] = morphSample[0][1];                            //use new lofi selected sample
        //   drumKitSamplePlaying[0]->setValue(morphSample[0][0]);             //point to new sample
        //   playMidi(instrSampleMidiNote[0], morphSample[0][0], MIDICHANNEL); //update rpi

        //   morphSample[0][1] = drumKitPatternPlaying[1]->getValue(); //use actual playing sample
        //   morphSample[1][1] = morphSample[0][1];                    //use actual playing sample

        //   morphSample[0][2] = drumKitPatternPlaying[2]->getValue(); //use actual playing sample
        //   morphSample[1][2] = morphSample[0][2];                    //use actual playing sample

        //   morphSample[0][3] = drumKitPatternPlaying[3]->getValue(); //use actual playing sample
        //   morphSample[1][3] = morphSample[0][3];                    //use actual playing sample

        //   morphSample[0][4] = drumKitPatternPlaying[4]->getValue(); //use actual playing sample
        //   morphSample[1][4] = morphSample[0][4];                    //use actual playing sample

        //   morphSample[0][5] = drumKitPatternPlaying[5]->getValue(); //use actual playing sample
        //   morphSample[1][5] = morphSample[0][5];                    //use actual playing sample

        //   break;
        // case 2: //lo fi hats
        //   randomSeed(millis());
        //   morphSample[0][0] = drumKitPatternPlaying[0]->getValue(); //use actual playing sample
        //   morphSample[1][0] = morphSample[0][0];                    //use actual playing sample

        //   morphSample[0][1] = drumKitPatternPlaying[1]->getValue(); //use actual playing sample
        //   morphSample[1][1] = morphSample[0][1];                    //use actual playing sample

        //   morphSample[0][2] = drumKitPatternPlaying[2]->getValue(); //use actual playing sample
        //   morphSample[1][2] = morphSample[0][2];                    //use actual playing sample

        //   morphSample[0][3] = drumKitPatternPlaying[3]->getValue(); //use actual playing sample
        //   morphSample[1][3] = morphSample[0][3];                    //use actual playing sample

        //   morphSample[0][4] = drumKitPatternPlaying[4]->getValue(); //use actual playing sample
        //   morphSample[1][4] = morphSample[0][4];                    //use actual playing sample

        //   morphSample[0][5] = lofiHihat[random(MAXLOFIHAT - 1)];            //use new lofi selected sample
        //   morphSample[1][5] = morphSample[0][5];                            //use new lofi selected sample
        //   morphPattern[0][5] = drumKitPatternPlaying[5]->getValue();        //use actual playing pattern
        //   morphPattern[1][5] = morphPattern[0][5];                          //use actual playing pattern
        //   drumKitSamplePlaying[5]->setValue(morphSample[0][5]);             //point to new sample
        //   playMidi(instrSampleMidiNote[5], morphSample[0][5], MIDICHANNEL); //update rpi

        //   morphSample[0][5] = drumKitPatternPlaying[5]->getValue(); //use actual playing sample
        //   morphSample[1][5] = morphSample[0][5];                    //use actual playing sample
        //        }
        SCREENupdateMorphBar(-1);
        morphingInstrument.setValue(0);
        lastSelectedMoodSelection = moodPointer;
        flagSelectMood = false;
        flagUpdateScreen = true;
      }
    }

    //update only one selected morphed pattern
    if (flagUpdateThisMorphedPattern != -1)
    {
      display.fillRect(0, patternScreenYinit + 5 + (flagUpdateThisMorphedPattern * 3), SCREEN_WIDTH, 3, BLACK);
      display.setTextColor(WHITE);
      for (int8_t step = 0; step < (MAXSTEPS - 2); step++) //for each step
      {
        boolean mustPrintStep = false;
        if (flagUpdateThisMorphedPattern == KICKCHANNEL)
        {
          if (referencePatternTableLow[flagUpdateThisMorphedDrumKit][step] == 1) //if it is an active step
            mustPrintStep = true;
        }
        else if ((flagUpdateThisMorphedPattern == SNAKERCHANNEL) ||
                 (flagUpdateThisMorphedPattern == CLAPCHANNEL) ||
                 (flagUpdateThisMorphedPattern == HATCHANNEL) ||
                 (flagUpdateThisMorphedPattern == OHHRIDECHANNEL))
        {
          if (referencePatternTableHi[flagUpdateThisMorphedDrumKit][step] == 1) //if it is an active step
            mustPrintStep = true;
        }
        else //pad table
        {
          if (referencePatternTablePad[flagUpdateThisMorphedDrumKit][step] == 1) //if it is an active step
            mustPrintStep = true;
        }
        if (mustPrintStep)
        {
          display.setCursor(step * 2, patternScreenYinit + (flagUpdateThisMorphedPattern * 3));
          display.print(F("."));
        }
      }
      SCREENupdateSampleAndPatternNumber(flagUpdateThisMorphedDrumKit);
      flagUpdateScreen = true;
      flagUpdateThisMorphedPattern = -1;
      flagUpdateThisMorphedDrumKit = -1;
    }
    if (flagUpdateSampleNumber != -1)
    {
      SCREENupdateSampleAndPatternNumber(flagUpdateSampleNumber);
      flagUpdateScreen = true;
      flagUpdateSampleNumber = -1;
    }
    if (flagUpdateScreen)
      display.display();
  }
  //very low cpu consumption tasks---------------------------------------------------------------------------------
  instrumentMute[(uint8_t)instrumentMuteQueue.advance()]->readPin(); //read one mute button
  encoderButtons[(uint8_t)encoderButtonQueue.advance()]->readPin();  //read on encodert button
  for (uint8_t i = 0; i < MAXENCODERS; i++)
    readEncoder(i);
  if (!encoderButtonMood.active() && interfaceEvent.debounced()) //mood selected....copy reference tables or create new mood in the morph area
  {
    interfaceEvent.debounce(); //block any other interface event
    flagSelectMood = true;
  }
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++) //search for ended triggers
    triggers[i]->compute();                    //shut them down
}

// void endTriggers()
// {
//   for (uint8_t i = 0; i < MAXINSTRUMENTS; i++) //search for ended triggers
//     digitalWrite(triggerPins[i], LOW);
//   Timer3.stop();
// }

//read queued encoder status
void readEncoder(uint8_t _queued)
{
  unsigned char result = encoders[_queued]->process();
  if (result)
  {
    int8_t _change = 0;
    if (result == 16)
      _change = 1;
    else
      _change = -1;

    switch (_queued)
    {
    case MOODENCODER:
      if (_change == 1)
      { //point to next mood on Db
        if (moodDb[1][moodPointer] != MAXMOODS)
          moodPointer = moodDb[1][moodPointer];
      }
      else
      { //point to previous mood on Db
        if (moodDb[0][moodPointer] != -1)
          moodPointer = moodDb[0][moodPointer];
      }
      //saves CPU if mood changed but still in the same position , in the start and in the end of the list
      if (lastSelectedMood != moodPointer)
      {
        flagUpdateMood = true;
        lastSelectedMood = moodPointer;
      }
      break;

    case MORPHENCODER:
      //CLOCKWISE : morph increased , point actual instrumentPointers morph area [1]
      if (_change == 1)
      {
        if (!morphingInstrument.isAtEnd())
          flagUpdateMorph = _change;
      }
      //if COUNTER-CLOCKWISE : morph decreased , point actual instrumentPointers morph area [0]
      else if (_change == -1)
      {
        if (!morphingInstrument.isAtInit())
          flagUpdateMorph = _change;
      }
      break;
      //all other instrument encoders
    default:
      //if encoder button pressed , CHANGE SAMPLE, , schedule to change screen too
      if (!encoderButtons[_queued]->active())
      {
        uint8_t realEncoder = _queued - 2;
        if (_change == -1)
          drumKitSamplePlaying[realEncoder]->reward();
        else
          drumKitSamplePlaying[realEncoder]->advance();
        // if ((realEncoder) <= (morphingInstrument.getValue()))
        morphArea[1][realEncoder] = drumKitSamplePlaying[realEncoder]->getValue();
        // else
        //   morphArea[0][realEncoder] = drumKitSamplePlaying[realEncoder]->getValue();
        playMidi(instrSampleMidiNote[realEncoder], drumKitSamplePlaying[realEncoder]->getValue(), MIDICHANNEL);
        flagUpdateSampleNumber = drumKitSamplePlaying[realEncoder]->getValue(); //update only the sample number
      }
      else //if not pressed , CHANGE PATTERN, schedule to change on screen too
      {
        uint8_t realEncoder = _queued - 2;
        if (_change == -1)
          drumKitPatternPlaying[realEncoder]->reward();
        else
          drumKitPatternPlaying[realEncoder]->advance();
        // if ((realEncoder) <= (morphingInstrument.getValue()))
        morphArea[1][realEncoder + MAXINSTRUMENTS] = drumKitPatternPlaying[realEncoder]->getValue();
        // else
        //   morphArea[0][realEncoder + MAXINSTRUMENTS] = drumKitPatternPlaying[realEncoder]->getValue();
        playMidi(instrPatternMidiNote[realEncoder], drumKitPatternPlaying[realEncoder]->getValue(), MIDICHANNEL);
        flagUpdateThisMorphedPattern = realEncoder;
        flagUpdateThisMorphedDrumKit = drumKitPatternPlaying[realEncoder]->getValue();
      }
      break;
    }
  }
}

//check if default screen wasnt show yet
void checkDefaultScreen()
{
  if (defaultScreenNotActiveYet)
  {
    defaultScreenNotActiveYet = false;
    SCREENdefault();
    SCREENupdateMood();
    SCREENupdateMorphBar(-1);
  }
}

void SCREENupdateMorphBar(int8_t _size)
{
  if (lastMorphBarGraphValue != _size)
  {
    display.fillRect(32, 0, 70, TEXTLINE_HEIGHT, BLACK);
    display.setCursor(32, 0);
    display.setTextColor(WHITE);
    for (int8_t i = 0; i < _size; i++)
      display.print(F(" |"));
    lastMorphBarGraphValue = _size;
  }
}

void SCREENupdateSampleAndPatternNumber(int8_t _val)
{
  display.fillRect(105, 0, 23, TEXTLINE_HEIGHT, BLACK);
  display.setCursor(105, 0);
  display.setTextColor(WHITE);
  display.print(_val);
}

void SCREENupdateMood()
{
  display.fillRect(0, TEXTLINE_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK); //black all screen from line 1 to end
  display.setCursor(0, TEXTLINE_HEIGHT);                                    //set cursor position
  display.print(moodName[moodPointer]);                                     //print mood name
  for (int8_t instr = 0; instr < MAXINSTRUMENTS; instr++)                   //for each instrument
  {
    for (int8_t step = 0; step < (MAXSTEPS - 2); step++) //for each step
    {
      if (drumKit[moodPointer][instr + MAXINSTRUMENTS] > 0) //skip if pattern is null or nothing
      {
        boolean mustPrintStep = false;
        if (moodPointer == KICKCHANNEL)
        {
          if (referencePatternTableLow[drumKit[moodPointer][instr + MAXINSTRUMENTS]][step] == 1) //if it is an active step
            mustPrintStep = true;
        }
        else if ((moodPointer == SNAKERCHANNEL) ||
                 (moodPointer == CLAPCHANNEL) ||
                 (moodPointer == HATCHANNEL) ||
                 (moodPointer == OHHRIDECHANNEL))
        {
          if (referencePatternTableHi[drumKit[moodPointer][instr + MAXINSTRUMENTS]][step] == 1) //if it is an active step
            mustPrintStep = true;
        }
        else
        {
          //pad table
          if (referencePatternTablePad[drumKit[moodPointer][instr + MAXINSTRUMENTS]][step] == 1) //if it is an active step
            mustPrintStep = true;
        }
        if (mustPrintStep)
        {
          display.setCursor(step * 2, patternScreenYinit + (instr * 3));
          display.print(F("."));
        }
      }
    }
  }
}

void SCREENdefault()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(F("Morph:"));
}

void SCREENwelcome()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("<-- select your mood"));
  display.println(F("    and morph it -->"));
}

void playMidi(uint8_t _note, uint8_t _velocity, uint8_t _channel)
{
#ifdef DOMIDI
  MIDI.sendNoteOn(_note, _velocity, _channel);
#endif

#ifdef OPENMIDIUSB
  noteOn(_channel, _velocity, _note); // Channel 0, middle C, normal velocity
  MidiUSB.flush();
#endif
}

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

// First parameter is the event type (0x0B = control change).
// Second parameter is the event type, combined with the channel.
// Third parameter is the control number number (0-119).
// Fourth parameter is the control value (0-127).

void controlChange(byte channel, byte control, byte value)
{
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}
