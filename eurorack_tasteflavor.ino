/*
Data flow diagram
                              +-------------+            +------------+            +---------+
                              |ref low table|            |ref hi table|            |pad table|
                              +-------------+            +------------+            +---------+
                                    |                           |                       |
                                    +---------------------------+-----------------------+
                                    |
                                    V
                              +-------------------+            +-------------------+          +-----------------------------+
                              |INSTRpatternPointer|            |INSTR2samplePointer| <--------|rpi available sample quantity|
                              +-------------------+            +-------------------+          +-----------------------------+
                                       |                                 |
                                       +---------------------------------+
                                                        |
                                                        V
                                                actual playing instr.
                        morphPattern <------>   drumKitPatternPlaying  <------> encoderPatternChange
                        morphSample  <------>   drumKitSamplePlaying   <------> encoderSampleChange

*/

#define DOMIDI true
#define OPENSERIAL false

#include <PantalaDefines.h>
#include <Switch.h>
#include <MicroDebounce.h>
#include <EventDebounce.h>
#include <Counter.h>
#include <Trigger.h>

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

//u8x8
// #include <Arduino.h>
// #include <U8x8lib.h>
// U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // OLEDs without Reset of the Display

#define MAXSTEPS 64          //max step sequence
#define MYCHANNEL 1          //midi channel for data transmission
#define MAXENCODERS 8        //total of encoders
#define MOODENCODER 0        //total of sample encoders
#define MORPHENCODER 1       //total of sample encoders
#define FIRSTSAMPLEENCODER 2 //total of sample encoders
#define MAXINSTRUMENTS 6     //total of sample encoders
#define TRIGGERINPIN 7       //total of sample encoders
#define MIDICHANNEL 10       //standart drum midi channel
#define MOODMIDINOTE 20      //midi note to mood message
#define MORPHMIDINOTE 21     //midi note to morph message

uint8_t INSTRsampleMidiNote[MAXINSTRUMENTS] = {10, 11, 12, 13, 14, 15};  //midi note to sample message
uint8_t INSTRpatternMidiNote[MAXINSTRUMENTS] = {20, 21, 22, 23, 24, 25}; //midi note to pattern message
EventDebounce interfaceEvent(200);                                       //min time in ms to accept another interface event
MicroDebounce stepLenght(125000);                                        //step sequence interval
MicroDebounce freeZone1(4000);                                           //first 4ms available toprocess light interface events
MicroDebounce triggerZone(2000);                                         //freeze cpu 2ms to close the triggers
MicroDebounce freeZone2(70000);                                          //big cpu time available to process screen and other stuff
#define L2CSAFETY 48000UL                                                //48ms L2C communication on oscilloscope =  38000UL
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
Trigger INSTR1trigger(5, true);
Trigger INSTR2trigger(3, true);
Trigger INSTR3trigger(16, true);
Trigger INSTR4trigger(18, true);
Trigger INSTR5trigger(23, true);
Trigger INSTR6trigger(25, true);
Trigger *INSTRtriggers[MAXINSTRUMENTS] = {&INSTR1trigger, &INSTR2trigger, &INSTR3trigger, &INSTR4trigger, &INSTR5trigger, &INSTR6trigger};

//encoders buttons
Switch encoderButtonMood(11, true);
Switch encoderButtonMorph(8, true);
Switch encoderButtonChannel1(52, true);
Switch encoderButtonChannel2(34, true);
Switch encoderButtonChannel3(46, true);
Switch encoderButtonChannel4(28, true);
Switch encoderButtonChannel5(40, true);
Switch encoderButtonChannel6(42, true);
Switch *encoderButtons[MAXENCODERS] = {&encoderButtonMood, &encoderButtonMorph, &encoderButtonChannel1, &encoderButtonChannel2, &encoderButtonChannel3, &encoderButtonChannel4, &encoderButtonChannel5, &encoderButtonChannel6};
Counter encoderButtonQueue(MAXENCODERS - 1);

//Switch triggerIn(TRIGGERINPIN);
Counter stepCounter(MAXSTEPS - 1);
Counter encoderQueue(MAXENCODERS - 1);

#define BASSCHANNEL 0
#define SNARECHANNEL 1
#define CLAPCHANNEL 2
#define HATCHANNEL 3
#define OHATCHANNEL 4
#define PADCHANNEL 5

#define MAXLOWTABLES 16
#define MAXHITABLES 26
#define MAXPADTABLES 18
boolean referencePatternTableLow[MAXLOWTABLES][MAXSTEPS] = {
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

boolean referencePatternTableHi[MAXHITABLES][MAXSTEPS] = {
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
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
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

boolean referencePatternTablePad[MAXPADTABLES][MAXSTEPS] = {
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
#define MAXMOODS 25 //max moods
String mood[MAXMOODS][2] = {{"", "0"},
                            {"Break Kicks 1", "0"},
                            {"4x4 mood 1", "0"},
                            {"4x4 mood 2", "0"},
                            {"4x4 mood 3", "0"},
                            {"4x4 mood 4", "0"},
                            {"4x4 mood 5", "0"},
                            {"4x4 mood 6", "0"},
                            {"Mood9", "0"},
                            {"Mood10", "0"},
                            {"Mood11", "0"},
                            {"Mood12", "0"},
                            {"Mood13", "0"},
                            {"mood14", "0"},
                            {"Import 1", "0"},
                            {"Import 1", "0"},
                            {"Import 2", "0"},
                            {"Import 3", "0"},
                            {"Import 4", "0"},
                            {"Import 1", "0"},
                            {"Import 2", "0"},
                            {"Import 3", "0"},
                            {"Add more Hats 1", "0"},
                            {"Lofi bass", "1"},
                            {"Lofi hat", "2"}};
int16_t moodDb[2][MAXMOODS] = {{-1, 3, 1, 0, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
                               {3, 2, 4, 1, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25}};
int16_t sampleKit[MAXMOODS][MAXINSTRUMENTS] = {
    {0, 0, 0, 0, 0, 0},
    {1, 2, 4, 3, 0, 0},
    {6, 4, 5, 3, 0, 0},
    {11, 4, 15, 13, 0, 0},
    {11, 5, 15, 18, 0, 0},
    {15, 5, 15, 18, 0, 0},
    {18, 2, 3, 13, 0, 0},
    {18, 2, 3, 0, 0, 0},
    {21, 4, 4, 13, 0, 0},
    {27, 5, 0, 13, 0, 0},
    {30, 5, 18, 17, 0, 0},
    {30, 0, 14, 14, 0, 0},
    {30, 0, 14, 14, 0, 0},
    {30, 0, 14, 14, 0, 0},
    {24, 6, 13, 6, 0, 0},
    {8, 8, 13, 5, 0, 0},
    {12, 25, 17, 20, 0, 0},
    {3, 13, 16, 17, 0, 0},
    {35, 15, 8, 7, 0, 0},
    {45, 11, 18, 14, 0, 0},
    {41, 16, 12, 7, 0, 0},
    {65, 22, 28, 3, 0, 0},
    {-1, -1, 20, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1}};
int16_t patternKit[MAXMOODS][MAXINSTRUMENTS] = {
    {0, 0, 0, 0, 0, 0},
    {7, 3, 5, 4, 0, 0},
    {3, 3, 6, 1, 0, 0},
    {3, 3, 6, 1, 0, 0},
    {3, 3, 6, 1, 0, 0},
    {3, 3, 6, 1, 0, 0},
    {3, 3, 9, 6, 0, 0},
    {3, 3, 9, 6, 0, 0},
    {5, 3, 8, 3, 0, 0},
    {3, 1, 9, 6, 0, 0},
    {3, 1, 9, 6, 0, 0},
    {3, 1, 9, 6, 0, 0},
    {3, 0, 9, 6, 0, 0},
    {7, 10, 11, 0, 0, 0},
    {8, 10, 2, 0, 0, 0},
    {9, 13, 12, 2, 0, 0},
    {10, 16, 15, 14, 0, 0},
    {11, 19, 18, 17, 0, 0},
    {12, 20, 21, 0, 0, 0},
    {13, 2, 22, 0, 0, 0},
    {14, 23, 22, 0, 0, 0},
    {15, 24, 25, 0, 0, 0},
    {-1, -1, 8, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1}};

//MORPH
uint8_t morphSample[2][MAXINSTRUMENTS] = {{0, 0, 0, 0, 0, 0},
                                          {0, 0, 0, 0, 0, 0}};
uint8_t morphPattern[2][MAXINSTRUMENTS] = {{0, 0, 0, 0, 0, 0},
                                           {0, 0, 0, 0, 0, 0}};
#define MAXLOFIKICK 11
uint8_t lofiKick[MAXLOFIKICK] = {0, 3, 4, 12, 17, 23, 25, 42, 43, 44, 45};
#define MAXLOFIHAT 10
uint8_t lofiHihat[MAXLOFIHAT] = {6, 16, 17, 22, 24, 33, 42, 50, 53, 62};

//mute buttons
Switch INSTR1mute(6, true);
Switch INSTR2mute(4, true);
Switch INSTR3mute(2, true);
Switch INSTR4mute(17, true);
Switch INSTR5mute(19, true);
Switch INSTR6mute(24, true);
Switch *INSTRmute[MAXINSTRUMENTS] = {&INSTR1mute, &INSTR2mute, &INSTR3mute, &INSTR4mute, &INSTR5mute, &INSTR6mute};
Counter INSTRmuteQueue(MAXINSTRUMENTS - 1);

//playing instrument pattern pointer
Counter INSTR1patternPointer(MAXLOWTABLES - 1);
Counter INSTR2patternPointer(MAXHITABLES - 1);
Counter INSTR3patternPointer(MAXHITABLES - 1);
Counter INSTR4patternPointer(MAXHITABLES - 1);
Counter INSTR5patternPointer(MAXHITABLES - 1);
Counter INSTR6patternPointer(MAXPADTABLES - 1);
Counter *drumKitPatternPlaying[MAXINSTRUMENTS] = {&INSTR1patternPointer, &INSTR2patternPointer, &INSTR3patternPointer, &INSTR4patternPointer, &INSTR5patternPointer, &INSTR6patternPointer};

//playing instrument sample pointer
Counter INSTR1samplePointer(5);
Counter INSTR2samplePointer(5);
Counter INSTR3samplePointer(5);
Counter INSTR4samplePointer(5);
Counter INSTR5samplePointer(5);
Counter INSTR6samplePointer(5);
Counter *drumKitSamplePlaying[MAXINSTRUMENTS] = {&INSTR1samplePointer, &INSTR2samplePointer, &INSTR3samplePointer, &INSTR4samplePointer, &INSTR5samplePointer, &INSTR6samplePointer};

//all encoder pins and related variables
uint8_t encPinA[MAXENCODERS] = {12, 9, 48, 30, 42, 24, 36, 15};
uint8_t encPinB[MAXENCODERS] = {13, 10, 50, 32, 44, 26, 38, 14};
int oldReadA[MAXENCODERS];
int oldReadB[MAXENCODERS];
int readA[MAXENCODERS];
int readB[MAXENCODERS];
int16_t newencoderValue[MAXENCODERS];
int16_t oldencoderValue[MAXENCODERS];

boolean playSteps = true;

uint16_t moodPointer = 0;
Counter morphingInstrument(MAXINSTRUMENTS);
uint8_t dontRepeatThisMood = 255;          //prevents to execute 2 times the same action
uint8_t dontRepeatThisMorph = 255;         //prevents to execute 2 times the same action
uint8_t dontRepeatThisMoodSelection = 255; //prevents to execute 2 times the same action
int8_t lastMorphBarGraphValue = 127;       //0 to MAXINSTRUMENTS possible values
uint32_t lastTick;                         //last time step counter was ticked
uint32_t tickInterval;                     //time in us between two ticks
boolean pleaseUpdateMood = false;          //schedule some screen update
int8_t pleaseUpdateMorph = 0;              //schedule some screen update
boolean pleaseSelectMood = false;          //schedule some screen update
int8_t pleaseUpdateOnePattern = -1;
boolean defaultScreenNotActiveYet = true;

void ISRtriggerIn();
void playMidi(uint8_t _note, uint8_t _velocity, uint8_t _channel);
void SCREENwelcome();
void SCREENdefault();
void SCREENupdateMood();
void SCREENprintStepDot(uint8_t _mood, uint8_t _i, uint8_t _j);
void SCREENupdateMorphBar(int8_t _size);
void encoderChanged(uint8_t _encoder, uint8_t _newValue, int8_t _change);
void readEncoder();
void checkDefaultScreen();
void startTriggerZone();
void startFreeZone2();

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

  //triggerIn.attachCallOnRising(ISRtriggerIn);
  for (uint8_t i = 0; i < MAXENCODERS; i++)
  {
    // set all encoder initial status
    pinMode(encPinA[i], INPUT);
    digitalWrite(encPinA[i], HIGH);
    pinMode(encPinB[i], INPUT);
    digitalWrite(encPinB[i], HIGH);
    oldReadA[i] = LOW;
    oldReadB[i] = LOW;
    readA[i] = LOW;
    readB[i] = LOW;
    newencoderValue[i] = 0;
    oldencoderValue[i] = 0;
  }

  //set these counters to not cyclabe
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    drumKitPatternPlaying[i]->setCyclable(false);
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    drumKitSamplePlaying[i]->setCyclable(false);
  morphingInstrument.setCyclable(false);

  //u8x8
  // u8x8.begin();
  // u8x8.setPowerSave(0);
  // u8x8.setFont(u8x8_font_chroma48medium8_r);
  // while (true)
  // {
  //   u8x8.println("Pantala labs");
  //   u8x8.refreshDisplay(); // only required for SSD1606/7
  //   u8x8.print("Taste & Flavor");
  //   u8x8.refreshDisplay(); // only required for SSD1606/7
  //   delay(200);
  // }

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
    tickInterval = micros() - lastTick;       //total pulse interval
    lastTick = micros();                      //save this tick time
    stepLenght.debounce();                    //start new step interval
    freeZone1.debounce();                     //frtees cpu to do small things
    uint8_t thisStep = stepCounter.advance(); //advance step pointer
    uint8_t noteVelocity = 0;                 //calculate final noteVelocity
    for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    {
      //if not muted and  this instrument step is TRUE
      if ((i <= 0) && (INSTRmute[i]->active() && (referencePatternTableLow[drumKitPatternPlaying[i]->getValue()][thisStep] == 1)))
      {
        noteVelocity = noteVelocity + powint(2, 5 - i);
        INSTRtriggers[i]->start();
      }
      //if not muted and  this instrument step is TRUE
      else if ((i <= 4) && (INSTRmute[i]->active() && (referencePatternTableHi[drumKitPatternPlaying[i]->getValue()][thisStep] == 1)))
      {
        noteVelocity = noteVelocity + powint(2, 5 - i);
        INSTRtriggers[i]->start();
      }
      //if not muted and  this instrument step is TRUE
      else if ((i == 5) && (INSTRmute[i]->active() && (referencePatternTablePad[drumKitPatternPlaying[i]->getValue()][thisStep] == 1)))
      {
        noteVelocity = noteVelocity + powint(2, 5 - i);
        INSTRtriggers[i]->start();
      }
    }
    //send midinote with binary coded triggers message
    if (noteVelocity != 0)
      playMidi(1, noteVelocity, MIDICHANNEL);
  }
  //if cpu available to large display updates
  if (!freeZone2.debounced())
  { //do screen stuff FIRST
    //update mood changes
    if (pleaseUpdateMood)
    {
      checkDefaultScreen();
      SCREENupdateMood();
      display.display();
      pleaseUpdateMood = false;
    }
    //update morph bar changes
    else if (pleaseUpdateMorph != 0)
    {
      if (pleaseUpdateMorph == 1)
      {
        morphingInstrument.advance();
        drumKitPatternPlaying[morphingInstrument.getValue() - 1]->setValue(morphPattern[1][morphingInstrument.getValue() - 1]);
        drumKitSamplePlaying[morphingInstrument.getValue() - 1]->setValue(morphSample[1][morphingInstrument.getValue() - 1]);
        playMidi(INSTRsampleMidiNote[morphingInstrument.getValue() - 1], drumKitSamplePlaying[morphingInstrument.getValue() - 1]->getValue(), MIDICHANNEL);
      }
      //if COUNTER-CLOCKWISE : morph decreased , point actual instrumentPointers morph area [0]
      else if (pleaseUpdateMorph == -1)
      {
        drumKitPatternPlaying[morphingInstrument.getValue() - 1]->setValue(morphPattern[0][morphingInstrument.getValue() - 1]);
        drumKitSamplePlaying[morphingInstrument.getValue() - 1]->setValue(morphSample[0][morphingInstrument.getValue() - 1]);
        playMidi(INSTRsampleMidiNote[morphingInstrument.getValue() - 1], drumKitSamplePlaying[morphingInstrument.getValue() - 1]->getValue(), MIDICHANNEL);
        morphingInstrument.reward(); //point to previous available instrument, until the first one
      }
      checkDefaultScreen();
      SCREENupdateMorphBar(morphingInstrument.getValue());
      display.display();
      pleaseUpdateMorph = 0;
    }
    //copy selected mood to morph area
    else if (pleaseSelectMood)
    {
      if (dontRepeatThisMoodSelection != moodPointer)
      {
        checkDefaultScreen();
        switch (mood[moodPointer][1].charAt(0))
        {
        // standard sample/pattern request
        case '0':
          for (uint8_t i = 0; i < MAXINSTRUMENTS; i++) //copy selected mood tables to morph area
          {
            //copy playing sample and pattern to morphArea 0
            morphSample[0][i] = drumKitSamplePlaying[i]->getValue();
            morphPattern[0][i] = drumKitPatternPlaying[i]->getValue();
            //if selected mood sample not an mood "add more"
            if (sampleKit[moodPointer][i] != -1)
              morphSample[1][i] = sampleKit[moodPointer][i]; //copy selected mood sample to morphArea 1
            else
              morphSample[1][i] = morphSample[0][i]; //use actual playing sample
            //if selected mood pattern not a mood "add more"
            if (patternKit[moodPointer][i] != -1)
              morphPattern[1][i] = patternKit[moodPointer][i]; //copy selected mood pattern to morphArea 1
            else
              morphPattern[1][i] = morphPattern[0][i]; //use actual playing pattern
          }
          break;
        // behavior command request
        case '1':
          morphSample[0][0] = lofiKick[random(MAXLOFIKICK - 1)];            //use new lofi selected sample
          morphSample[1][0] = morphSample[0][0];                            //use new lofi selected sample
          drumKitSamplePlaying[0]->setValue(morphSample[0][0]);             //point to new sample
          playMidi(INSTRsampleMidiNote[0], morphSample[0][0], MIDICHANNEL); //update rpi
          for (uint8_t i = 1; i < MAXINSTRUMENTS; i++)                      //copy actual mood tables to morph area
          {
            morphPattern[0][i] = drumKitPatternPlaying[i]->getValue(); //use actual playing pattern
            morphPattern[1][i] = morphPattern[0][i];                   //use actual playing pattern
          }
          break;
        case '2':
          morphSample[0][0] = drumKitSamplePlaying[0]->getValue(); //use new lofi selected sample
          morphSample[1][0] = morphSample[0][0];                   //use new lofi selected sample
          for (uint8_t i = 1; i < 5; i++)                          //copy actual mood tables to morph area
          {
            morphSample[0][i] = lofiHihat[random(MAXLOFIHAT - 1)];            //use new lofi selected sample
            morphSample[1][i] = morphSample[0][i];                            //use new lofi selected sample
            morphPattern[0][i] = drumKitPatternPlaying[i]->getValue();        //use actual playing pattern
            morphPattern[1][i] = morphPattern[0][i];                          //use actual playing pattern
            drumKitSamplePlaying[i]->setValue(morphSample[0][i]);             //point to new sample
            playMidi(INSTRsampleMidiNote[i], morphSample[0][i], MIDICHANNEL); //update rpi
          }
          morphPattern[0][5] = drumKitPatternPlaying[5]->getValue(); //use actual playing pattern
          morphPattern[1][5] = morphPattern[0][5];                   //use actual playing pattern
          break;
        }
        SCREENupdateMorphBar(-1);
        display.display();
        morphingInstrument.setValue(0);
        dontRepeatThisMoodSelection = moodPointer;
        pleaseSelectMood = false;
      }
    }
    else if (pleaseUpdateOnePattern != -1)
    {
      display.setTextColor(BLACK);
      for (int8_t i = 0; i < (MAXSTEPS - 2); i++) //for each step
      {
        display.setCursor(i * 2, 15 + (pleaseUpdateOnePattern * 3));
        display.print(F("."));
      }
      display.setTextColor(WHITE);
      for (int8_t i = 0; i < (MAXSTEPS - 2); i++) //for each step
      {
        boolean mustPrintDot = false;
        if (pleaseUpdateOnePattern == 0) //low table
        {
          if (referencePatternTableLow[drumKitPatternPlaying[pleaseUpdateOnePattern]->getValue()][i] == 1) //if it is an active step
            mustPrintDot = true;
        }
        else if (pleaseUpdateOnePattern < 4) //hi table
        {
          if (referencePatternTableHi[drumKitPatternPlaying[pleaseUpdateOnePattern]->getValue()][i] == 1) //if it is an active step
            mustPrintDot = true;
        }
        else
        {
          //pad table
        }
        if (mustPrintDot)
        {
          display.setCursor(i * 2, 15 + (pleaseUpdateOnePattern * 3));
          display.print(F("."));
        }
      }
      display.display();
      pleaseUpdateOnePattern = -1;
    }
  }
  //very low cpu consumption tasks , could be done anytime
  //read one encoder at time any time
  if (!freeZone1.debounced() || !freeZone2.debounced())
  {
    readEncoder(encoderQueue.advance());
    //if cpu available to large display updates
    encoderButtons[(uint8_t)encoderButtonQueue.advance()]->readPin(); //read each encoder button
    INSTRmute[(uint8_t)INSTRmuteQueue.advance()]->readPin();          //read each mute button
    if (!encoderButtonMood.active() && interfaceEvent.debounced())    //mood selected....copy reference tables or create new mood in the morph area
    {
      interfaceEvent.debounce(); //block any other interface event
      pleaseSelectMood = true;
    }
  }
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++) //search for ended triggers
    INSTRtriggers[i]->compute();               //shut them down
}

//deal with any encoder change
void encoderChanged(uint8_t _encoder, uint8_t _newValue, int8_t _change)
{
  switch (_encoder)
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
    if (dontRepeatThisMood != moodPointer)
    {
      pleaseUpdateMood = true;
      dontRepeatThisMood = moodPointer;
    }
    break;

  case MORPHENCODER:
    //playMidi((MORPHMIDINOTE, _newValue, MIDICHANNEL);
    //CLOCKWISE : morph increased , point actual instrumentPointers morph area [1]
    if (_change == 1)
    {
      if (!morphingInstrument.isAtEnd())
        pleaseUpdateMorph = _change;
    }
    //if COUNTER-CLOCKWISE : morph decreased , point actual instrumentPointers morph area [0]
    else if (_change == -1)
    {
      if (!morphingInstrument.isAtInit())
        pleaseUpdateMorph = _change;
    }
    break;
    //all other instrument encoders
  default:
    //if encoder button pressed , change instrument
    if (!encoderButtons[_encoder]->active())
    {
      if (_change == -1)
        drumKitSamplePlaying[_encoder - 2]->reward();
      else
        drumKitSamplePlaying[_encoder - 2]->advance();
      morphSample[0][_encoder - 2] = drumKitSamplePlaying[_encoder - 2]->getValue();
      playMidi(INSTRsampleMidiNote[_encoder - 2], _newValue, MIDICHANNEL);
    }
    else //if not pressed , change step pattern, schedule to change pattern on screen too
    {
      if (_change == -1)
        drumKitPatternPlaying[_encoder - 2]->reward();
      else
        drumKitPatternPlaying[_encoder - 2]->advance();
      morphPattern[0][_encoder - 2] = drumKitPatternPlaying[_encoder - 2]->getValue();
      playMidi(INSTRpatternMidiNote[_encoder - 2], drumKitPatternPlaying[_encoder - 2]->getValue(), MIDICHANNEL);
      pleaseUpdateOnePattern = _encoder - 2;
    }
    break;
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
    display.fillRect(40, 0, SCREEN_WIDTH, TEXTLINE_HEIGHT, BLACK);
    display.setCursor(40, 0);
    display.setTextColor(WHITE);
    for (int8_t i = 0; i < _size; i++)
      display.print(F(" O"));
    lastMorphBarGraphValue = _size;
  }
}

void SCREENupdateMood()
{
  display.fillRect(0, TEXTLINE_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK); //black all screen from line 1 to end
  display.setCursor(0, TEXTLINE_HEIGHT);                                    //set cursor position
  display.print(mood[moodPointer][0]);                                      //print mood name
  for (int8_t j = 0; j < 4; j++)                                            //for each instrument
  {
    for (int8_t i = 0; i < (MAXSTEPS - 2); i++) //for each step
    {
      if (patternKit[moodPointer][j] > 0) //skip if pattern is null or nothing
      {
        if (referencePatternTableHi[patternKit[moodPointer][j]][i] == 1) //if it is an active step
        {
          display.setCursor(i * 2, 15 + (j * 3));
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
  display.println(F("   and morph it -->>"));
}

void playMidi(uint8_t _note, uint8_t _velocity, uint8_t _channel)
{

#ifdef DOMIDI
  MIDI.sendNoteOn(_note, _velocity, _channel);
#endif
  if (OPENSERIAL)
  {
    // Serial.print(_note);
    // Serial.print(" - ");
    // Serial.print(_velocity);
    // Serial.print(" - ");
    // Serial.print(_channel);
    // Serial.println(".");
  }
}

//read queued encoder status
void readEncoder(uint8_t _queued)
{
  int8_t newRead;
  newRead = compare4EncoderStates(_queued);
  //if there was any change
  if (newRead != 0)
  {
    newencoderValue[_queued] += newRead;
    if (oldencoderValue[_queued] != newencoderValue[_queued]) //avoid repeat direction change repeat like:  9,10,11,11,10,9,...
    {
      oldencoderValue[_queued] = newencoderValue[_queued];
      if ((newencoderValue[_queued] % 4) == 0)
        encoderChanged(_queued, newencoderValue[_queued] / 4, newRead);
    }
  }
}

// manually read and compare the four possible states to figure out what if changed
int8_t compare4EncoderStates(int _encoder)
{
  readA[_encoder] = digitalRead(encPinA[_encoder]);
  readB[_encoder] = digitalRead(encPinB[_encoder]);
  int changeValue = 0;
  if (readA[_encoder] != oldReadA[_encoder])
  {
    if (readA[_encoder] == LOW)
    {
      if (readB[_encoder] == LOW)
        changeValue = -1;
      else
        changeValue = 1;
    }
    else
    {
      if (readB[_encoder] == LOW)
        changeValue = 1;
      else
        changeValue = -1;
    }
  }
  if (readB[_encoder] != oldReadB[_encoder])
  {
    if (readB[_encoder] == LOW)
    {
      if (readA[_encoder] == LOW)
        changeValue = 1;
      else
        changeValue = -1;
    }
    else
    {
      if (readA[_encoder] == LOW)
        changeValue = -1;
      else
        changeValue = 1;
    }
  }
  oldReadA[_encoder] = readA[_encoder];
  oldReadB[_encoder] = readB[_encoder];
  return changeValue;
}
