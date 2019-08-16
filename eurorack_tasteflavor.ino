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
#include <Adafruit_SSD1306.h>

MIDI_CREATE_DEFAULT_INSTANCE();

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET 4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

boolean DEBUG = 0;

#define MAXMOODS 13          //max steps combinations
#define MAXSTEPS 32          //max step sequence
#define MYCHANNEL 1          //midi channel for data transmission
#define MAXENCODERS 8        //total of encoders
#define MOODENCODER 0        //total of sample encoders
#define MORPHENCODER 1       //total of sample encoders
#define FIRSTSAMPLEENCODER 2 //total of sample encoders
#define MAXINSTRUMENTS 6     //total of sample encoders
#define TRIGGERINPIN 7       //total of sample encoders
#define MIDICHANNEL 1
#define MOODMIDINOTE 20    //midi note to mood message
#define MORPHMIDINOTE 21   //midi note to morph message
#define L2CBANDWIDTH 50000 //40ms L2C communication /???WHY ?????

uint8_t INSTRsampleMidiNote[MAXINSTRUMENTS] = {10, 11, 12, 13, 14, 15};  //midi note to sample message
uint8_t INSTRpatternMidiNote[MAXINSTRUMENTS] = {20, 21, 22, 23, 24, 25}; //midi note to pattern message
EventDebounce interfaceEvent(200);                                       //min time in ms to accept another interface event
MicroDebounce stepInterval(125000);                                      //step sequence interval
MicroDebounce cpuBusy(5500);                                             //discard the first 5ms to close the triggers
MicroDebounce cpuAvailable(60000);                                       //calculated to do not mix essential tasks with interface tasks
/*
      +---+                                   |
      |   |                                   |
______|   |___________________________________|
                              |<-safetyidle->|

             |<-cpuAvailable->|
      |<--->|cpuBusy                      
      |<----------stepInterval----------->|
 */

//trigger out pins
Trigger INSTR1trigger(5);
Trigger INSTR2trigger(3);
Trigger INSTR3trigger(16);
Trigger INSTR4trigger(18);
Trigger INSTR5trigger(23);
Trigger INSTR6trigger(25);
Trigger *INSTRtriggers[MAXINSTRUMENTS] = {&INSTR1trigger, &INSTR2trigger, &INSTR3trigger, &INSTR4trigger, &INSTR5trigger, &INSTR6trigger};

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
#define MAXHITABLES 10
#define MAXPADTABLES 18

Switch INSTR1mute(6, true);
Switch INSTR2mute(4, true);
Switch INSTR3mute(2, true);
Switch INSTR4mute(17, true);
Switch INSTR5mute(19, true);
Switch INSTR6mute(24, true);
Switch *INSTRmute[MAXINSTRUMENTS] = {&INSTR1mute, &INSTR2mute, &INSTR3mute, &INSTR4mute, &INSTR5mute, &INSTR6mute};
Counter INSTRmuteQueue(MAXINSTRUMENTS - 1);

Counter INSTR1patternPointer(MAXLOWTABLES - 1);
Counter INSTR2patternPointer(MAXHITABLES - 1);
Counter INSTR3patternPointer(MAXHITABLES - 1);
Counter INSTR4patternPointer(MAXHITABLES - 1);
Counter INSTR5patternPointer(MAXHITABLES - 1);
Counter INSTR6patternPointer(MAXPADTABLES - 1);
Counter *drumKitPatternPlaying[MAXINSTRUMENTS] = {&INSTR1patternPointer, &INSTR2patternPointer, &INSTR3patternPointer, &INSTR4patternPointer, &INSTR5patternPointer, &INSTR6patternPointer};

//fill with max available samples on pi
Counter INSTR1samplePointer(5);
Counter INSTR2samplePointer(5);
Counter INSTR3samplePointer(5);
Counter INSTR4samplePointer(5);
Counter INSTR5samplePointer(5);
Counter INSTR6samplePointer(5);
Counter *drumKitSamplePlaying[MAXINSTRUMENTS] = {&INSTR1samplePointer, &INSTR2samplePointer, &INSTR3samplePointer, &INSTR4samplePointer, &INSTR5samplePointer, &INSTR6samplePointer};

uint8_t encPinA[MAXENCODERS] = {13, 9, 48, 30, 42, 24, 36, 15};
uint8_t encPinB[MAXENCODERS] = {12, 10, 50, 32, 44, 26, 38, 14};
int oldReadA[MAXENCODERS];
int oldReadB[MAXENCODERS];
int readA[MAXENCODERS];
int readB[MAXENCODERS];
int16_t newencoderValue[MAXENCODERS];
int16_t oldencoderValue[MAXENCODERS];

boolean playSteps = true;

boolean referencePatternTableLow[MAXLOWTABLES][32] = {
    //, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _},
    //, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //..
    {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0}, //4x4
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}, //4x4 + 1
    {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}, //4x4 + 1
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //tum tum...
    {1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //
    {0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0}, //
    {1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0}};
boolean referencePatternTableHi[MAXHITABLES][32] = {
    //, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _},
    //, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //..
    {0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0}, //tss
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0}, //tah
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}, //tss......ts...
    {1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}, //ti di di ....ti .....ti
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0}, //......tei tei tei
    {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0}, //tah tah ath ath
    {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, //tah tah ath ath
    {1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0}, //ti di di rápido
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}  //tststststststst
};

boolean referencePatternTablePad[MAXPADTABLES][32] = {
    //, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _},
    //, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //..
    {1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0},
    {1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0},
    {1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1},
    {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
    {1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0},
    {1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0},
    {1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1},
    {1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0},
    {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1},
    {1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0},
    {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0}};

//uint8_t actualMood = 0;
String moodName[MAXMOODS] = {"", "Perfect4x4", "Mood3", "Mood4", "Mood5", "Mood6", "Mood7", "Mood8", "Mood9", //
                             "Mood10", "Mood11", "Mood12", "Mood13"};

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

uint8_t morphStatus[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
uint8_t morphSample[2][MAXINSTRUMENTS] = {{0, 0, 0, 0, 0, 0},
                                          {0, 0, 0, 0, 0, 0}};
uint8_t morphPattern[2][MAXINSTRUMENTS] = {{0, 0, 0, 0, 0, 0},
                                           {0, 0, 0, 0, 0, 0}};

Counter moodPointer(MAXMOODS - 1);
Counter morphingInstrument(5);
int8_t lastChange = 0; //-1 / 0 / 1 possible values
uint8_t lastQueuedMoodValue = 0;
uint8_t lastDisplayedMorphBarGraph = 1; //0 to MAXINSTRUMENTS possible values
int8_t morphedInstruments = -1;         //0 to MAXINSTRUMENTS possible values
uint32_t lastTick;                      //last time step counter was ticked
boolean cpuCalculationDone;             //time space between cpuAvailable and the next pulse
boolean pleaseUpdateMoodOnScreen;       //schedule some screen update
boolean pleaseResetMorphActivity;       //schedule some screen update

void ISRtriggerIn();
void playMidi(uint8_t _note, uint8_t _velocity, uint8_t _channel);
void SCREENdefaultScreen();
void SCREENupdateMoodValue(uint8_t _new);
void SCREENupdateMorphBarGraph(int8_t _size);
void encoderChanged(uint8_t _encoder, uint8_t _newValue, int8_t _change);
void readEncoder();
void resetMorphActivity();

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
    // set encoder behavior
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

  //set these counters not cyclabe
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    drumKitPatternPlaying[i]->setCyclable(false);
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    drumKitSamplePlaying[i]->setCyclable(false);
  moodPointer.setCyclable(false);
  morphingInstrument.setCyclable(false);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
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
  SCREENdefaultScreen();
  SCREENupdateMoodValue(0);
  SCREENupdateMorphBarGraph(-1);
  display.display();

  // while (true)
  // {
  //   display.fillRect(0, 29, SCREEN_WIDTH, 9, BLACK);
  //   display.setCursor(0, 29);
  //   display.print(random(12));
  //   display.display();
  //   delay(200);
  // }
}

void loop()
{
  //PRIORITY 0 - please play next step
  if (playSteps && stepInterval.debounced())
  {
    stepInterval.debounce();                  //start new step interval
    cpuBusy.debounce();                       //start cpu busy until the triggers get closed
    cpuCalculationDone = false;               //flags to do nothiung until next step comes
    uint8_t thisStep = stepCounter.advance(); //advance step pointer
    uint8_t noteVelocity = 0;                 //calculate noteVelocity
    for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    {
      //if not muted and  this instrument step is TRUE
      if ((i <= 0) && (INSTRmute[i]->active() && (referencePatternTableLow[drumKitPatternPlaying[i]->getValue()][thisStep] == 1)))
      {
        noteVelocity = noteVelocity + powint(2, 5 - i);
        INSTRtriggers[i]->start();
      }
      else if ((i <= 4) && (INSTRmute[i]->active() && (referencePatternTableHi[drumKitPatternPlaying[i]->getValue()][thisStep] == 1)))
      {
        noteVelocity = noteVelocity + powint(2, 5 - i);
        INSTRtriggers[i]->start();
      }
      else if ((i == 5) && (INSTRmute[i]->active() && (referencePatternTablePad[drumKitPatternPlaying[i]->getValue()][thisStep] == 1)))
      {
        noteVelocity = noteVelocity + powint(2, 5 - i);
        INSTRtriggers[i]->start();
      }
    }
    if (noteVelocity != 0)
      playMidi(1, noteVelocity, MIDICHANNEL);
  }
  //PRIORITY 1 - calculate available CPU
  if (cpuBusy.debounced() && cpuAvailable.debounced() && !cpuCalculationDone)
  {
    cpuAvailable.debounce(micros() - lastTick - 2000);
    lastTick = micros();
    cpuCalculationDone = true;
  }
  //PRIORITY 2 - do all other tasks
  if (!cpuAvailable.debounced())
  {
    //do screen stuff FIRST
    if (pleaseUpdateMoodOnScreen)
    {
      SCREENupdateMoodValue(moodPointer.getValue());
      display.display();
      pleaseUpdateMoodOnScreen = false;
    }
    else if (pleaseResetMorphActivity)
    {
      resetMorphActivity(); //reset all morph activity
      display.display();    //refresh
    }

    encoderButtons[(uint8_t)encoderButtonQueue.advance()]->readPin(); //read encoder button

    INSTRmute[(uint8_t)INSTRmuteQueue.advance()]->readPin(); //read mute button

    readEncoder((uint8_t)encoderQueue.advance()); //read encoder rotation

    //mood selected....copy reference tables to morph area
    if (!encoderButtonMood.active() && interfaceEvent.debounced())
    {
      interfaceEvent.debounce();                   //block any other interface event
      for (uint8_t i = 0; i < MAXINSTRUMENTS; i++) //copy selected mood to morph area
      {
        morphSample[0][i] = drumKitSamplePlaying[i]->getValue();
        morphPattern[0][i] = drumKitPatternPlaying[i]->getValue();
        morphSample[1][i] = sampleKit[moodPointer.getValue()][i];
        morphPattern[1][i] = patternKit[moodPointer.getValue()][i];
      }
      pleaseResetMorphActivity = true;
    }
  }
  //close triggers
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    INSTRtriggers[i]->compute();
}

void encoderChanged(uint8_t _encoder, uint8_t _newValue, int8_t _change)
{
  switch (_encoder)
  {
  case MOODENCODER:
    if (_change == 1)
      moodPointer.reward();
    else
      moodPointer.advance();
    if (lastQueuedMoodValue != moodPointer.getValue())
    {
      //
      pleaseUpdateMoodOnScreen = true;
      lastQueuedMoodValue = moodPointer.getValue();
    }
    break;

  case MORPHENCODER:
    //playMidi((MORPHMIDINOTE, _newValue, MIDICHANNEL);
    //CLOCKWISE : morph increased , point actual instrumentPointers morph area [1]
    if (_change == 1)
    {
      //if all instruments already morphed, exit
      if (morphingInstrument.getValue() == (MAXINSTRUMENTS - 1))
      {
        lastChange = _change;
        break;
      }
      //if this instrument was already morphed and next instrument is allowed
      if ((morphStatus[morphingInstrument.getValue()] == 1) &&   //
          (morphingInstrument.getValue() < (MAXINSTRUMENTS - 1)) //
      )
      {
        morphingInstrument.advance(); //point to next available instrument
      }
      morphedInstruments++;
      morphStatus[morphingInstrument.getValue()] = 1;
      drumKitPatternPlaying[morphingInstrument.getValue()]->setValue(morphPattern[1][morphingInstrument.getValue()]);
      drumKitSamplePlaying[morphingInstrument.getValue()]->setValue(morphSample[1][morphingInstrument.getValue()]);
      playMidi(INSTRsampleMidiNote[morphingInstrument.getValue()], drumKitSamplePlaying[morphingInstrument.getValue()]->getValue(), MIDICHANNEL);
    }
    //if COUNTER-CLOCKWISE : morph decreased , point actual instrumentPointers morph area [0]
    else if (_change == -1)
    {
      //if it is the first intrument and it is already morphed back
      if ((morphingInstrument.getValue() == 0) && (morphStatus[morphingInstrument.getValue()] == 0))
      {
        SCREENupdateMorphBarGraph(-1);
        lastChange = _change;
        break;
      }
      //if this instrument already morphed, point to prdecessor instrument
      if (morphStatus[morphingInstrument.getValue()] == 0)
      {
        morphingInstrument.reward(); //point to previous available instrument, until the first one
      }
      morphedInstruments--;
      morphStatus[morphingInstrument.getValue()] = 0;
      drumKitPatternPlaying[morphingInstrument.getValue()]->setValue(morphPattern[0][morphingInstrument.getValue()]);
      drumKitSamplePlaying[morphingInstrument.getValue()]->setValue(morphSample[0][morphingInstrument.getValue()]);
      playMidi(INSTRsampleMidiNote[morphingInstrument.getValue()], drumKitSamplePlaying[morphingInstrument.getValue()]->getValue(), MIDICHANNEL);
    }
    lastChange = _change;
    SCREENupdateMorphBarGraph(morphedInstruments);
    display.display();
    break;

  default:                                   //all instrument encoders
    if (!encoderButtons[_encoder]->active()) //change instrument
    {
      if (_change == -1)
        drumKitSamplePlaying[_encoder - 2]->reward();
      else
        drumKitSamplePlaying[_encoder - 2]->advance();
      morphSample[0][_encoder - 2] = drumKitSamplePlaying[_encoder - 2]->getValue();
      playMidi(INSTRsampleMidiNote[_encoder - 2], _newValue, MIDICHANNEL);
    }
    else //change step pattern, do not send any midi
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

void resetMorphActivity()
{
  morphedInstruments = -1;
  morphingInstrument.setValue(0);
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    morphStatus[i] = 0;
  SCREENupdateMorphBarGraph(-1);
}

void SCREENupdateMorphBarGraph(int8_t _size)
{
  if (lastDisplayedMorphBarGraph != _size)
  {
    display.setCursor(40, 0);
    display.setTextColor(BLACK);
    // for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    //   display.print(F("|"));
    display.fillRect(40, 0, 40, 9, BLACK);
    display.setCursor(40, 0);
    display.setTextColor(WHITE);
    for (int8_t i = 0; i <= _size; i++)
      display.print(F("|"));
    lastDisplayedMorphBarGraph = _size;
  }
}

void SCREENupdateMoodValue(uint8_t _new)
{
  display.fillRect(0, 29, SCREEN_WIDTH, 9, BLACK);
  display.setCursor(0, 29);
  display.setTextColor(WHITE);
  display.print(moodName[_new]);
}

void SCREENdefaultScreen()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(F("Morph:"));
  display.setCursor(0, 20);
  display.print(F("Next mood:"));
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
