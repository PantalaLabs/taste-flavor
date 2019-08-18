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

#include <PantalaDefines.h>
#include <Switch.h>
#include <MicroDebounce.h>
#include <EventDebounce.h>
#include <Counter.h>
#include <Trigger.h>
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();
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

boolean DEBUG = 0;

#define MAXMOODS 13          //max steps combinations
#define MAXSTEPS 64          //max step sequence
#define MYCHANNEL 1          //midi channel for data transmission
#define MAXENCODERS 8        //total of encoders
#define MOODENCODER 0        //total of sample encoders
#define MORPHENCODER 1       //total of sample encoders
#define FIRSTSAMPLEENCODER 2 //total of sample encoders
#define MAXINSTRUMENTS 6     //total of sample encoders
#define TRIGGERINPIN 7       //total of sample encoders
#define MIDICHANNEL 1
#define MOODMIDINOTE 20  //midi note to mood message
#define MORPHMIDINOTE 21 //midi note to morph message

#define L2CSAFETY 50000 //50ms L2C communication /???WHY ?????
#define CPUBUSY 5100    //50ms L2C communication /???WHY ?????

uint8_t INSTRsampleMidiNote[MAXINSTRUMENTS] = {10, 11, 12, 13, 14, 15};  //midi note to sample message
uint8_t INSTRpatternMidiNote[MAXINSTRUMENTS] = {20, 21, 22, 23, 24, 25}; //midi note to pattern message
EventDebounce interfaceEvent(200);                                       //min time in ms to accept another interface event
MicroDebounce stepInterval(125000);                                      //step sequence interval
MicroDebounce cpuBusy(CPUBUSY);                                          //discard the first 5ms to close the triggers
MicroDebounce cpuAvailable(60000);                                       //calculated to do not mix essential tasks with interface tasks
/*
      +---+                                             |
      |   |                                             |
______|   |_____________________________________________|
      |<-->|CPUBUSY                     |<--L2CSAFETY-->|
           |<-------cpuAvailable------->|
      |<------------------stepInterval----------------->|
*/

//trigger out pins
Trigger INSTR1trigger(5);
Trigger INSTR2trigger(3);
Trigger INSTR3trigger(16);
Trigger INSTR4trigger(18);
Trigger INSTR5trigger(23);
Trigger INSTR6trigger(25);
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

// #define SUBCHANNEL 6
// #define KICKCHANNEL 4
// #define SNARECHANNEL 2
// #define CLAPCHANNEL 17
// #define HATCHANNEL 19
// #define PADCHANNEL 14

#define MAXLOWTABLES 8
#define MAXHITABLES 11
#define MAXPADTABLES 18

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
    {1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0}};
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
    {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0}

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

//MOOD
String moodName[MAXMOODS] = {"", "Break Kicks 1", "4x4 mood 1", "4x4 mood 2", "4x4 mood 3", "4x4 mood 4", "4x4 mood 5", "4x4 mood 6", "Mood9", //
                             "Mood10", "Mood11", "Mood12", "Mood13"};
int16_t moodDb[2][MAXMOODS] = {{-1, 3, 1, 0, 2, 4, 5, 6, 7, 8, 9, 10, 11},
                               {3, 2, 4, 1, 5, 6, 7, 8, 9, 10, 11, 12, 13}};
uint8_t sampleKit[MAXMOODS][MAXINSTRUMENTS] = {{0, 0, 0, 0, 0, 0},
                                               {1, 2, 4, 3, 0, 0},
                                               {6, 4, 5, 3, 0, 0},
                                               {11, 4, 15, 13, 0, 0},
                                               {11, 5, 15, 18, 0, 0},
                                               {15, 5, 15, 18, 0, 0},
                                               {18, 2, 3, 13, 0, 0},
                                               {18, 2, 3, 0, 0, 0},
                                               {21, 4, 4, 13, 0, 0},
                                               {27, 5, 0, 13, 0, 0},
                                               {25, 5, 0, 13, 0, 0},
                                               {30, 5, 18, 17, 0, 0},
                                               {30, 0, 14, 14, 0, 0}};
uint8_t patternKit[MAXMOODS][MAXINSTRUMENTS] = {{0, 0, 0, 0, 0, 0},
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
                                                {3, 0, 9, 6, 0, 0}};

//MORPH
uint8_t morphSample[2][MAXINSTRUMENTS] = {{0, 0, 0, 0, 0, 0},
                                          {0, 0, 0, 0, 0, 0}};
uint8_t morphPattern[2][MAXINSTRUMENTS] = {{0, 0, 0, 0, 0, 0},
                                           {0, 0, 0, 0, 0, 0}};

uint16_t moodPointer = 0;
Counter morphingInstrument(MAXINSTRUMENTS);
uint8_t dontRepeatThisMood = 255;          //prevents to execute 2 times the same action
uint8_t dontRepeatThisMorph = 255;         //prevents to execute 2 times the same action
uint8_t dontRepeatThisMoodSelection = 255; //prevents to execute 2 times the same action
int8_t lastMorphBarGraphValue = 127;       //0 to MAXINSTRUMENTS possible values
uint32_t lastTick;                         //last time step counter was ticked
uint32_t tickInterval;                     //time in us between two ticks
boolean cpuCalculationDone;                //time space between cpuAvailable and the next pulse
boolean pleaseUpdateMood = false;          //schedule some screen update
boolean pleaseUpdateMorph = false;         //schedule some screen update
boolean pleaseSelectMood = false;          //schedule some screen update
boolean defaultScreenNotActiveYet = true;

void ISRtriggerIn();
void playMidi(uint8_t _note, uint8_t _velocity, uint8_t _channel);
void SCREENwelcome();
void SCREENdefault();
void SCREENupdateMood();
void SCREENupdateMorphBar(int8_t _size);
void encoderChanged(uint8_t _encoder, uint8_t _newValue, int8_t _change);
void readEncoder();
void checkDefaultScreen();

void ISRtriggerIn()
{
  //playSteps = !playSteps;
}

void setup()
{
  // digitalWrite(SDA, LOW);
  // digitalWrite(SCL, LOW);

  if (DEBUG)
    Serial.begin(9600);
  //triggerIn.attachCallOnRising(ISRtriggerIn);
  MIDI.begin();
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
  if (playSteps && stepInterval.debounced())
  {
    stepInterval.debounce();                  //start new step interval
    cpuBusy.debounce();                       //puts cpu on idle until the triggers get closed
    cpuCalculationDone = false;               //flags to remember that all cpu calculations was done
    tickInterval = micros() - lastTick;       //total pulse interval
    lastTick = micros();                      //save this tick time
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
  //PRIORITY 1 - calculate available CPU after cpuBusy ended
  if (cpuBusy.debounced() && !cpuCalculationDone)
  {
    cpuAvailable.debounce(tickInterval - CPUBUSY - L2CSAFETY);
    cpuCalculationDone = true;
  }
  //PRIORITY 2 - do interface readings and screen updates
  //read one encoder at time
  readEncoder(encoderQueue.advance());
  //mood selected....copy reference tables to morph area
  if (!encoderButtonMood.active() && interfaceEvent.debounced())
  {
    interfaceEvent.debounce(); //block any other interface event
    pleaseSelectMood = true;
  }
  //if cpu available to large display updates
  if (!cpuAvailable.debounced())
  {
    //do screen stuff FIRST
    //update mood changes
    if (pleaseUpdateMood)
    {
      checkDefaultScreen();
      SCREENupdateMood();
      display.display();
      pleaseUpdateMood = false;
    }
    //update morph bar changes
    else if (pleaseUpdateMorph)
    {
      checkDefaultScreen();
      SCREENupdateMorphBar(morphingInstrument.getValue());
      display.display();
      pleaseUpdateMorph = false;
    }
    //copy selected mood to morph area
    else if (pleaseSelectMood)
    {
      if (dontRepeatThisMoodSelection != moodPointer)
      {
        checkDefaultScreen();
        for (uint8_t i = 0; i < MAXINSTRUMENTS; i++) //copy selected mood tables to morph area
        {
          morphSample[0][i] = drumKitSamplePlaying[i]->getValue();
          morphPattern[0][i] = drumKitPatternPlaying[i]->getValue();
          morphSample[1][i] = sampleKit[moodPointer][i];
          morphPattern[1][i] = patternKit[moodPointer][i];
        }
        SCREENupdateMorphBar(-1);
        display.display();
        morphingInstrument.setValue(0);
        dontRepeatThisMoodSelection = moodPointer;
        pleaseSelectMood = false;
      }
    }
  }
  //very low cpu time consumption tasks , could be done anytime
  encoderButtons[(uint8_t)encoderButtonQueue.advance()]->readPin(); //read each encoder button
  INSTRmute[(uint8_t)INSTRmuteQueue.advance()]->readPin();          //read each mute button
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)                      //close triggers
    INSTRtriggers[i]->compute();                                    //shut them down
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
      morphingInstrument.advance();
      drumKitPatternPlaying[morphingInstrument.getValue() - 1]->setValue(morphPattern[1][morphingInstrument.getValue() - 1]);
      drumKitSamplePlaying[morphingInstrument.getValue() - 1]->setValue(morphSample[1][morphingInstrument.getValue() - 1]);
      playMidi(INSTRsampleMidiNote[morphingInstrument.getValue() - 1], drumKitSamplePlaying[morphingInstrument.getValue() - 1]->getValue(), MIDICHANNEL);
    }
    //if COUNTER-CLOCKWISE : morph decreased , point actual instrumentPointers morph area [0]
    else if (_change == -1)
    {
      if (morphingInstrument.getValue() > 0)
      {
        drumKitPatternPlaying[morphingInstrument.getValue() - 1]->setValue(morphPattern[0][morphingInstrument.getValue() - 1]);
        drumKitSamplePlaying[morphingInstrument.getValue() - 1]->setValue(morphSample[0][morphingInstrument.getValue() - 1]);
        playMidi(INSTRsampleMidiNote[morphingInstrument.getValue() - 1], drumKitSamplePlaying[morphingInstrument.getValue() - 1]->getValue(), MIDICHANNEL);
      }
      morphingInstrument.reward(); //point to previous available instrument, until the first one
    }
    //prevents to update screen unnecessarilly with the same value
    if (dontRepeatThisMorph != morphingInstrument.getValue())
      pleaseUpdateMorph = true;
    break;

    //all instrument encoders
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
    else //if not pressed , change step pattern
    {
      if (_change == -1)
        drumKitPatternPlaying[_encoder - 2]->reward();
      else
        drumKitPatternPlaying[_encoder - 2]->advance();
      morphPattern[0][_encoder - 2] = drumKitPatternPlaying[_encoder - 2]->getValue();
      playMidi(INSTRpatternMidiNote[_encoder - 2], drumKitPatternPlaying[_encoder - 2]->getValue(), MIDICHANNEL);
    }
    break;
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
  display.print(moodName[moodPointer]);                                     //print mood name
  for (int8_t j = 0; j < 4; j++)                                            //for each instrument
  {
    for (int8_t i = 0; i < (MAXSTEPS - 2); i++) //for each step
    {
      if (referencePatternTableHi[patternKit[moodPointer][j]][i] == 1) //if it is an active step
      {
        display.setCursor(i * 2, 15 + (j * 3));
        display.print(F("."));
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
  display.setCursor(0, TEXTLINE_HEIGHT);
}

void playMidi(uint8_t _note, uint8_t _velocity, uint8_t _channel)
{
  if (!DEBUG)
  {
    MIDI.sendNoteOn(_note, _velocity, _channel);
  }
  else
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
