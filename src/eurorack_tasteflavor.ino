/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create melodies
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#define DO_SERIAL true

#include <Arduino.h>
#include <PantalaDefines.h>
#include <EventDebounce.h>
#include <Rotary.h>
#include <DueTimer.h>
#include <AnalogInput.h>

//main objects

#include "Deck.h"
#include "Deck.h"
Deck *deck[2];

#include "Patterns.h"
Patterns *pattern;

#include "Melody.h"
Melody *melody;

#include <wavTrigger.h>
wavTrigger wTrig;

//ladder menu
#define MAXPARAMETERS 8
#define COMMANDSMP 1
#define COMMANDBPM 2
#define COMMANDLAT 3
#define COMMANDTAP 4
#define COMMANDUND 5
#define COMMANDSIL 6
#define COMMANDERS 7
#define COMMANDTRL 8
AnalogInput ladderMenu(A6);
uint8_t laddderMenuOption;
int laddderMenuSeparators[MAXPARAMETERS + 1] = {23, 65, 105, 200, 300, 480, 610, 715, 1023};
EventDebounce laddderMenuReadInterval(60);

//display
#define I2C_ADDRESS 0x3C
#define DISPLAY_WIDTH 128       // display width, in pixels
#define DISPLAY_HEIGHT 64       // display height, in pixels
#define TEXTLINE_HEIGHT 9       // text line height, in pixels
#define DISPLAYUPDATETIME 35000 //microsseconds to update display
#define DOTGRIDINIT 36
#define GRIDPATTERNHEIGHT 4

//adafruit
#include <Adafruit_SSD1306.h>
#define OLED_RESET 45 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, OLED_RESET);

//sequencer
#define MAXMELODYPARMS 6 //max ppotentiometer arameters
#define MAXSTEPS 64      //max step sequence
#define MAXENCODERS 8    //total of encoders
#define MOODENCODER 0    //MOOD encoder id
#define CROSSENCODER 1   //CROSS encoder id
#define MAXINSTRUMENTS 6 //total of instruments
uint8_t queuedMelodyParameter;
Filter *paramFilter[MAXMELODYPARMS]; //add filter for each param pot

//PINS
#define TRIGGERINPIN 51
#define RESETINPIN 53
#define RXPIN 17
#define TXPIN 18

#define SDAPIN 20
#define SCLPIN 21

#define ENCPINAMOOD 13
#define ENCPINBMOOD 12
#define ENCBUTPINMOOD 11

#define ENCPINACROSS 10
#define ENCPINBCROSS 9
#define ENCBUTPINCROSS 8

#define ENCPINAINSTR1 38
#define ENCPINBINSTR1 36
#define ENCBUTPININSTR1 40

#define ENCPINAINSTR2 32
#define ENCPINBINSTR2 30
#define ENCBUTPININSTR2 34

#define ENCPINAINSTR3 44
#define ENCPINBINSTR3 42
#define ENCBUTPININSTR3 46

#define ENCPINAINSTR4 14
#define ENCPINBINSTR4 15
#define ENCBUTPININSTR4 22

#define ENCBUTPININSTR5 52
#define ENCPINAINSTR5 50
#define ENCPINBINSTR5 48

#define ENCBUTPININSTR6 28
#define ENCPINAINSTR6 26
#define ENCPINBINSTR6 24

#define TRIGOUTPATTERNPIN1 7

#define TRIGOUTPIN1 5
#define TRIGOUTPIN2 3
#define TRIGOUTPIN3 16
#define TRIGOUTPIN4 29
#define TRIGOUTPIN5 23
#define TRIGOUTPIN6 27

#define MELODYPARAMSET 47

#define ACTIONPININSTR1 6
#define ACTIONPININSTR2 4
#define ACTIONPININSTR3 2
#define ACTIONPININSTR4 17
#define ACTIONPININSTR5 31
#define ACTIONPININSTR6 25

#define ENCBUTMOOD 0
#define ENCBUTCROSS 1
#define ENCBUTINSTR1 2
#define ENCBUTINSTR2 3
#define ENCBUTINSTR3 4
#define ENCBUTINSTR4 5
#define ENCBUTINSTR5 6
#define ENCBUTINSTR6 7

EventDebounce info(30);
uint8_t infoState;

//time related and bpm
volatile uint32_t u_lastTick;                   //last time tick was called
volatile uint32_t u_tickInterval = 1000000;     //tick interval
boolean updateDisplayUpdateLatencyComp = false; //flag to update latency compensation
int32_t u_LatencyComp = 1100;                   //default latency compensation keep the 100 microsseconds there
int32_t u_LatencyCompStep = 1000;               //latency compensation amount update step
#define u_LatencyCompLimit 20000                //latency compensation + and - limit
volatile uint32_t sampleChangeWindowEndTime;
uint16_t bpm = 125;
uint32_t u_bpm = 0;
uint8_t powArray[6] = {1, 2, 4, 8, 16, 32};

#define MAXGATELENGHTS 8
#define DEFAULTGATELENGHT 5000
#define EXTENDEDGATELENGHT 40000
boolean thisDeck = 0;
//#define notthisDeck(y) (return ((y==0)?1:0)

//triggers
volatile int8_t gateLenghtCounter = 0;
uint8_t triggerPins[MAXINSTRUMENTS] = {TRIGOUTPIN1, TRIGOUTPIN2, TRIGOUTPIN3, TRIGOUTPIN4, TRIGOUTPIN5, TRIGOUTPIN6};

//encoders buttons : mood, cross, instruments
uint8_t encoderButtonPins[MAXENCODERS] = {ENCBUTPINMOOD, ENCBUTPINCROSS, ENCBUTPININSTR1, ENCBUTPININSTR2, ENCBUTPININSTR3, ENCBUTPININSTR4, ENCBUTPININSTR5, ENCBUTPININSTR6};
boolean encoderButtonState[MAXENCODERS] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t instrActionPins[MAXINSTRUMENTS] = {ACTIONPININSTR1, ACTIONPININSTR2, ACTIONPININSTR3, ACTIONPININSTR4, ACTIONPININSTR5, ACTIONPININSTR6}; //pins
boolean instrActionState[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
uint8_t sampleButtonModifier[MAXENCODERS] = {0, 0, ENCBUTINSTR2, ENCBUTINSTR1, ENCBUTINSTR4, ENCBUTINSTR3, ENCBUTINSTR6, ENCBUTINSTR5}; //this points to each instrument sample change button modifier

EventDebounce interfaceEvent(200); //min time in ms to accept another interface event
EventDebounce melodieUpdate(70);   //min time in ms read new melody parameter

Rotary MOODencoder(ENCPINAMOOD, ENCPINBMOOD);
Rotary CROSSencoder(ENCPINACROSS, ENCPINBCROSS);
Rotary instr1encoder(ENCPINAINSTR1, ENCPINBINSTR1);
Rotary instr2encoder(ENCPINAINSTR2, ENCPINBINSTR2);
Rotary instr3encoder(ENCPINAINSTR3, ENCPINBINSTR3);
Rotary instr4encoder(ENCPINAINSTR4, ENCPINBINSTR4);
Rotary instr5encoder(ENCPINAINSTR5, ENCPINBINSTR5);
Rotary instr6encoder(ENCPINAINSTR6, ENCPINBINSTR6);
Rotary *encoders[MAXENCODERS] = {&MOODencoder, &CROSSencoder, &instr1encoder, &instr2encoder, &instr3encoder, &instr4encoder, &instr5encoder, &instr6encoder};

boolean updateDisplayBrowseMood = false;      //schedule some display update
boolean updateDisplaySelectMood = false;      //schedule some display update
boolean updateDisplayUpdateBpm = false;       //update bpm
int16_t selectedMood = 0;                     //actual selected mood
int16_t lastSelectedMood = 255;               //prevents to execute 2 times the same action
int8_t previousMood = 0;                      //previous mood name
int8_t lastCrossBarGraphValue = 127;          //0 to MAXINSTRUMENTS possible values
int8_t updateDisplayUpdateCross = 0;          //schedule some display update
int8_t updateDisplayInstrPattern = -1;        //update only one instrument pattern
int8_t updateDisplayChangePlayingSample = -1; //update sample on rasp pi
int8_t updateDisplayUpdategateLenght = -1;    //update gate lenght on right upper corner
int8_t updateDisplayEraseInstrumentPattern = -1;
int8_t updateDisplayTapInstrumentPattern = -1;
int8_t updateDisplayRollbackInstrumentTap = -1;
boolean updateDisplayModifierPressed = false;
boolean defaultDisplayNotActiveYet = true;
boolean externalClockSource = 0; //0=internal , 1=external

EventDebounce switchBackToInternalClock(3000);
//sequencer
volatile int8_t stepCount = 0;
int8_t queuedRotaryEncoder = 0;

//mood
#define NOSAMPLE 0
#define NOPATTERN 1

#define MAXMOODS 5 //max valid moods including muted mood
String moodKitName[MAXMOODS] = {"", "P.Labs-Empty Room", "P.Labs-Choke", "P.Labs-April23", "Carlos Pires-Drama"};
uint8_t moodKitPatterns[MAXMOODS][MAXINSTRUMENTS] = {
    {1, 1, 1, 1, 1, 1}, //reserved MUTE = 1
    {2, 2, 2, 2, 2, 2}, //P.Labs-Empty Room
    {2, 3, 3, 4, 3, 1}, //P.Labs-Choke
    {2, 3, 4, 2, 3, 1}, //P.Labs-April23
    {2, 5, 5, 2, 5, 4}  //Carlos Pires-Drama
};

//decks
#define RAM 0
#define ROM 1
int8_t crossfader = 0;
int8_t lastCrossfadedValue = MAXINSTRUMENTS;

void setup()
{
  analogWriteResolution(12);
  analogReadResolution(10);
  pinMode(DAC0, OUTPUT);
  ladderMenu.setMenu(laddderMenuSeparators, MAXPARAMETERS + 1);

#ifdef DO_SERIAL
  Serial.begin(9600);
  Serial.println("Debugging..");
#endif

  deck[0] = new Deck(MAXMOODS, MAXINSTR1PATTERNS, MAXINSTR2PATTERNS, MAXINSTR3PATTERNS, MAXINSTR4PATTERNS, MAXINSTR5PATTERNS, MAXINSTR6PATTERNS);
  deck[1] = new Deck(MAXMOODS, MAXINSTR1PATTERNS, MAXINSTR2PATTERNS, MAXINSTR3PATTERNS, MAXINSTR4PATTERNS, MAXINSTR5PATTERNS, MAXINSTR6PATTERNS);
  pattern = new Patterns();
  melody = new Melody(64);

  for (uint8_t i = 0; i < MAXMELODYPARMS; i++)
    paramFilter[i] = new Filter(6);

  // pinMode(TRIGGERINPIN, INPUT);
  // attachInterrupt(digitalPinToInterrupt(TRIGGERINPIN), ISRswitchToExternal, RISING);

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
  {
    pinMode(triggerPins[i], OUTPUT);
    digitalWrite(triggerPins[i], LOW);
  }

  //start pattern trigger out
  pinMode(TRIGOUTPATTERNPIN1, OUTPUT);
  digitalWrite(TRIGOUTPATTERNPIN1, HIGH);
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
  {
    digitalWrite(triggerPins[i], HIGH);
    analogWrite(DAC0, i * 512);
    delay(250);
  }
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
  {
    digitalWrite(triggerPins[i], LOW);
    analogWrite(DAC0, (MAXINSTRUMENTS - i) * 512);
    delay(250);
  }
  analogWrite(DAC0, 0);
  digitalWrite(TRIGOUTPATTERNPIN1, LOW);

  //if mood encoder button pressed , do a full DAC voltage test
  while (!digitalRead(encoderButtonPins[0]))
  {
    analogWrite(DAC0, 4095);
    delay(5000);
    analogWrite(DAC0, 0);
    delay(5000);
  }

  // WAV Trigger startup at 57600
  delay(1000);
  wTrig.start();
  delay(10);
  wTrig.stopAllTracks();
  wTrig.masterGain(-9);
  wTrig.samplerateOffset(0);
  wTrig.setReporting(true);
  delay(100);

  //setup display
  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS))
  {
#ifdef DO_SERIAL
    // Address 0x3D for 128x64
    Serial.begin(9600);
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed
#endif
  }
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // white text
  display.cp437(true);
  display.setCursor(0, 0);
  display.println(F("Pantala Labs"));
  display.println(F("Taste & Flavor"));
  display.println(F(""));

  char gWTrigVersion[21];
  if (wTrig.getVersion(gWTrigVersion, VERSION_STRING_LEN))
  {
    display.println(F(gWTrigVersion));
    display.print(wTrig.getNumTracks());
    display.println(F(" samples"));
  }
  else
    display.println(F("No WT communication"));
  wTrig.setReporting(false);
  display.display();
  delay(2000);
  displayWelcome();
  display.display();

  //internal clock
  u_bpm = bpm2micros4ppqn(bpm);
  Timer3.attachInterrupt(ISRfireTimer3);
  Timer3.start(u_bpm);
  Timer4.attachInterrupt(ISRfireTimer4);
  Timer5.attachInterrupt(ISRendTriggers);
}

// void ISRmanualFireTimer3()
// {
//   noInterrupts();
//   externalClockSource = true;
//   Timer3.detachInterrupt();
//   Timer3.stop();
//   u_tickInterval = micros() - u_lastTick;
//   u_lastTick = micros();
//   if (u_LatencyComp >= 0)
//     Timer4.start(u_LatencyComp);
//   else
//     Timer4.start(u_tickInterval + u_LatencyComp);
//   interrupts();
//   switchBackToInternalClock.debounce();
// }

//base clock
void ISRfireTimer3()
{
  noInterrupts();
  Timer3.stop();
  u_tickInterval = micros() - u_lastTick;
  u_lastTick = micros();
  if (u_LatencyComp >= 0)
    Timer4.start(u_LatencyComp);
  else
    Timer4.start(u_tickInterval + u_LatencyComp);
  Timer3.start(bpm2micros4ppqn(bpm));
  interrupts();
}

//return if this instrument belongs to this or not to this deck
boolean crossfadedDeck(uint8_t _instr)
{
  return (_instr < crossfader) ? thisDeck : !thisDeck;
}

//shifted clock and everything to step sequencer related
void ISRfireTimer4()
{
  noInterrupts();
  Timer5.start(DEFAULTGATELENGHT);                         //start 5ms first trigger timer
  Timer4.stop();                                           //stop timer shift
  for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++) //for each instrument
  {
    boolean targetDeck;
    targetDeck = crossfadedDeck(instr);
    //if this step wasnt permanently muted
    if (!deck[targetDeck]->permanentMute[instr]
        //and wasnt manually muted
        && !instrActionState[instr]
        // //and wasnt manually tapped
        && !deck[targetDeck]->tappedStep[instr]
        //and this is an real pattern step
        && deck[targetDeck]->pattern->isThisStepActive(instr, deck[targetDeck]->deckPatterns[instr]->getValue(), stepCount))
    {
      digitalWrite(triggerPins[instr], HIGH);
      //uint16_t sampleId = deck[targetDeck]->id ;
      uint16_t sampleId = deck[targetDeck]->deckSamples[instr]->getValue();
      wTrig.trackGain(sampleId, -6);
      wTrig.trackLoad(sampleId);
    }
    deck[targetDeck]->tappedStep[instr] = false; //reset any tapped step
  }
  wTrig.resumeAllInSync();
  interrupts();
  analogWrite(DAC0, melody->getNote());
  gateLenghtCounter = 0;
  stepCount++;
  if (stepCount >= MAXSTEPS)
  {
    stepCount = 0;
    melody->resetStepCounter();
  }
  sampleChangeWindowEndTime = (micros() + u_tickInterval - DISPLAYUPDATETIME);
}

//close all trigger
//  _ ______ _______ _______ _______
//_| |      |       |       |       |______
void ISRendTriggers()
{
  //compare all triggers times
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
  {
    if (deck[crossfadedDeck(i)]->gateLenghtSize[i] <= gateLenghtCounter)
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

//allows to change samples only upo to after 2/3 of the tick interval
//to avoid to change sample the same time it was triggered
boolean safetyZone()
{
  return (micros() < sampleChangeWindowEndTime);
}

//read queued encoder status
void readRotaryEncoder(uint8_t _queued)
{
  unsigned char result = encoders[_queued]->process();
  if ((result == 16) || (result == 32))
  {
    int8_t encoderChange = 0;
    if (result == 16)
      encoderChange = 1;
    else
      encoderChange = -1;

    if (_queued == MOODENCODER)
    {
      selectedMood += encoderChange;
      selectedMood = constrain(selectedMood, 0, MAXMOODS - 1);
      if (lastSelectedMood != selectedMood)
      {
        updateDisplayBrowseMood = true;
        lastSelectedMood = selectedMood;
      }
    }
    else if (_queued == CROSSENCODER)
    {
      //if cross button AND cross button are pressed and cross rotate change timer shift
      if (laddderMenuOption == COMMANDLAT)
      {
        u_LatencyComp += encoderChange * u_LatencyCompStep;
        u_LatencyComp = constrain(u_LatencyComp, -u_LatencyCompLimit, u_LatencyCompLimit);
        updateDisplayUpdateLatencyComp = true;
      }
      //if cross button is pressed and cross rotate CHANGE BPM
      else if (laddderMenuOption == COMMANDBPM)
      {
        updateDisplayUpdateBpm = true;
        bpm += encoderChange;
        u_bpm = bpm2micros4ppqn(bpm);
      }
      else
      { //or else , cross change
        crossfader += encoderChange;
        crossfader = constrain(crossfader, 0, MAXINSTRUMENTS);
        updateDisplayUpdateCross = encoderChange;
      }
    }
    else
    {
      //all other instrument encoders
      uint8_t _instrum = _queued - 2; //discard mood and morph indexes
      //sample change
      if (laddderMenuOption == COMMANDSMP)
      {
        if (encoderChange == -1)
          deck[thisDeck]->deckSamples[_instrum]->safeChange(-MAXINSTRUMENTS);
        else
          deck[thisDeck]->deckSamples[_instrum]->safeChange(MAXINSTRUMENTS);
        updateDisplayChangePlayingSample = _instrum;
      }
      //gate lenght change
      else if (laddderMenuOption == COMMANDTRL)
      {
        deck[thisDeck]->changeGateLenghSize(_instrum, encoderChange);
        updateDisplayUpdategateLenght = _instrum;
      }
      //pattern change
      else if (noOneEncoderButtonIsPressed())
      {
        if (encoderChange == -1)
          deck[thisDeck]->deckPatterns[_instrum]->reward();
        else
          deck[thisDeck]->deckPatterns[_instrum]->advance();
        updateDisplayInstrPattern = _instrum; //flags to update this instrument on display
      }
    }
  }
}

//check if default display wasnt show yet
void checkDefaultDisplay()
{
  if (defaultDisplayNotActiveYet)
  {
    defaultDisplayNotActiveYet = false;
    display.clearDisplay();
    display.drawRect(0, 0, 60, TEXTLINE_HEIGHT - 2, WHITE);
    displayShowBrowsedMood();
    displayShowCrossBar(-1);
  }
}

void displayWelcome()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("<-- select your mood"));
  display.println(F("    and cross it -->"));
}

void displayShowCrossBar(int8_t _size) //update crossing status / 6 steps of 10 pixels each
{
  if (lastCrossBarGraphValue != _size)
  {
    display.fillRect(1, 1, 58, TEXTLINE_HEIGHT - 4, BLACK);
    for (int8_t i = 0; i < _size; i++)
      display.fillRect(10 * i, 0, 10, TEXTLINE_HEIGHT - 2, WHITE);
    lastCrossBarGraphValue = _size;
  }
}

void displayShowCornerInfo(uint8_t _parm, int16_t _val) //update display right upper corner with the actual sample or pattern number
{
  String rightCornerInfo[5] = {"pat", "smp", "bpm", "ms", "len"};
  display.fillRect(70, 0, DISPLAY_WIDTH - 70, TEXTLINE_HEIGHT, BLACK);
  display.setCursor(70, 0);
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
    display.print((DEFAULTGATELENGHT + (deck[crossfadedDeck(_val)]->gateLenghtSize[_val] * EXTENDEDGATELENGHT)) / 1000);
  }
}

void displayEraseLineAndSetText(uint8_t _line)
{
  display.fillRect(0, _line * TEXTLINE_HEIGHT, DISPLAY_WIDTH, TEXTLINE_HEIGHT - 1, BLACK);
  display.setCursor(0, _line * TEXTLINE_HEIGHT); //set cursor position
}

void displayUpdateLineArea(uint8_t _line, String _content)
{
  displayEraseLineAndSetText(_line);
  display.print(_content); //print previous mood name
}

uint8_t lastBrowsedMood;
void displayShowBrowsedMood() //update almost all bottom display area with the name of the selected mood and all 6 available instruments
{
  if (lastBrowsedMood != selectedMood)
  {
    lastBrowsedMood = selectedMood;
    displayUpdateLineArea(3, moodKitName[selectedMood]);
    for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++) //for each instrument
      displayShowInstrPattern(instr, ROM);
  }
}

void displayShowInstrPattern(uint8_t _instr, boolean _src)
{
  displayEraseInstrumentPattern(_instr);
  for (int8_t step = 0; step < (MAXSTEPS - 1); step++) //for each step
  {
    //if browsed mood (ROM) or individual pattern browse (RAM)
    if (((_src == ROM) && (pattern->getStep(_instr, moodKitPatterns[selectedMood][_instr], step))) ||
        ((_src == RAM) && (deck[thisDeck]->pattern->getStep(_instr, deck[thisDeck]->deckPatterns[_instr]->getValue(), step))))
      display.fillRect(step * 2, DOTGRIDINIT + (_instr * GRIDPATTERNHEIGHT), 2, GRIDPATTERNHEIGHT - 1, WHITE);
  }
}

//erase exatctly one line pattern
void displayEraseInstrumentPattern(uint8_t _instr)
{
  display.fillRect(0, DOTGRIDINIT + (_instr * GRIDPATTERNHEIGHT), DISPLAY_WIDTH, GRIDPATTERNHEIGHT - 1, BLACK);
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

void loop()
{
  //flags if any display update will be necessary in this cycle
  boolean updateDisplay = false;

  //read queued interruptred encoders
  queuedRotaryEncoder++;
  if (queuedRotaryEncoder == MAXENCODERS)
    queuedRotaryEncoder = 0;
  readRotaryEncoder(queuedRotaryEncoder);

  //check if there is any async display update scheduled

  //mood browse
  if (updateDisplayBrowseMood)
  {
    checkDefaultDisplay();
    displayShowBrowsedMood();
    updateDisplayBrowseMood = false;
    updateDisplay = true;
  }
  //right upper corner with bpm value
  else if (updateDisplayUpdateBpm)
  {
    updateDisplayUpdateBpm = false;
    displayShowCornerInfo(2, bpm);
    updateDisplay = true;
  }
  //cross bar changes inside sample update window
  //this could be took off if using Tsunami
  else if ((updateDisplayUpdateCross != 0) && safetyZone())
  {
    checkDefaultDisplay();
    displayShowCrossBar(crossfader);
    updateDisplayUpdateCross = 0;
    updateDisplay = true;
  }
  //copy selected mood to new deck inside sample update window
  else if (updateDisplaySelectMood && safetyZone())
  {
    checkDefaultDisplay();
    //before load new mood, prepare current deck to leave it the same way user customized
    for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++)
    {
      //if this instrument belongs to the other deck
      if (crossfadedDeck(instr) == !thisDeck)
      {
        //if this instrument was left behind on the other deck , save it on new
        if (lastCrossfadedValue > crossfader)
        {
          deck[thisDeck]->deckSamples[instr]->setValue(deck[!thisDeck]->deckSamples[instr]->getValue());
          deck[thisDeck]->deckPatterns[instr]->setValue(deck[!thisDeck]->deckPatterns[instr]->getValue());
          deck[thisDeck]->permanentMute[instr] = false;
          deck[thisDeck]->gateLenghtSize[instr] = deck[!thisDeck]->gateLenghtSize[instr];
        }
        else //or discard
        {
          deck[thisDeck]->deckSamples[instr]->reset();
          deck[thisDeck]->deckPatterns[instr]->reset();
          deck[thisDeck]->permanentMute[instr] = true;
          deck[thisDeck]->gateLenghtSize[instr] = 0;
        }
      }
      lastCrossfadedValue = crossfader;
    }
    //change decks
    crossfader = 0;
    thisDeck = !thisDeck;
    deck[thisDeck]->cue(selectedMood,
                        moodKitPatterns[selectedMood][0], //patterns
                        moodKitPatterns[selectedMood][1], //
                        moodKitPatterns[selectedMood][2], //
                        moodKitPatterns[selectedMood][3], //
                        moodKitPatterns[selectedMood][4], //
                        moodKitPatterns[selectedMood][5]  //
    );
    displayUpdateLineArea(1, moodKitName[previousMood]);
    displayUpdateLineArea(2, moodKitName[selectedMood]);
    displayShowCrossBar(-1);
    previousMood = selectedMood;
    updateDisplaySelectMood = false;
    updateDisplay = true;
  }

  //if only one intrument pattern changed
  else if (updateDisplayInstrPattern != -1)
  {
    displayShowInstrPattern(updateDisplayInstrPattern, RAM);
    displayUpdateLineArea(3, "Custom");
    displayShowCornerInfo(0, deck[thisDeck]->deckPatterns[updateDisplayInstrPattern]->getValue());
    updateDisplay = true;
    updateDisplayInstrPattern = -1;
  }
  //if sampler changed and there is available time to update screen
  else if ((updateDisplayChangePlayingSample != -1) && safetyZone())
  {
    displayUpdateLineArea(3, moodKitName[deck[thisDeck]->deckSamples[updateDisplayChangePlayingSample]->getValue() / 6]);
    displayShowCornerInfo(1, deck[thisDeck]->deckSamples[updateDisplayChangePlayingSample]->getValue());
    updateDisplay = true;
    updateDisplayChangePlayingSample = -1;
  }
  //if latency compensation changed
  else if (updateDisplayUpdateLatencyComp)
  {
    displayShowCornerInfo(3, (u_LatencyComp - 100) / 1000);
    updateDisplay = true;
    updateDisplayUpdateLatencyComp = false;
  }
  //if gate lenght changed
  else if (updateDisplayUpdategateLenght != -1)
  {
    displayShowCornerInfo(4, updateDisplayUpdategateLenght);
    updateDisplay = true;
    updateDisplayUpdategateLenght = -1;
  }
  //if erase pattern
  else if (updateDisplayEraseInstrumentPattern != -1)
  {
    displayEraseInstrumentPattern(updateDisplayEraseInstrumentPattern); //clear pattern from display
    updateDisplay = true;
    updateDisplayEraseInstrumentPattern = -1;
  }
  //if step was tapped
  else if (updateDisplayTapInstrumentPattern != -1)
  {
    displayShowInstrPattern(updateDisplayTapInstrumentPattern, RAM); //update display with new inserted step
    updateDisplay = true;
    updateDisplayTapInstrumentPattern = -1;
  }
  else if (updateDisplayRollbackInstrumentTap != -1)
  {
    displayShowInstrPattern(updateDisplayRollbackInstrumentTap, RAM); //update display with removed step
    updateDisplay = true;
    updateDisplayRollbackInstrumentTap = -1;
  }

  //if any display update
  if (updateDisplay)
    display.display();

  //read interface inputs ===========================================================================================================
  //read all encoders buttons (invert boolean state to make easy future comparation)
  for (int8_t i = 0; i < MAXENCODERS; i++)
    encoderButtonState[i] = !digitalRead(encoderButtonPins[i]);

  //if new mood was selected....copy reference tables or create new mood in the cross area
  if (onlyOneEncoderButtonIsPressed(ENCBUTMOOD) && interfaceEvent.debounced())
  {
    interfaceEvent.debounce(1000); //block any other interface event
    updateDisplaySelectMood = true;
  }

  //read all action buttons (invert boolean state to make easy future comparation)
  for (int8_t i = 0; i < MAXINSTRUMENTS; i++)
  {
    instrActionState[i] = !digitalRead(instrActionPins[i]); //read action instrument button

    //if any instrument should be tapped - ladder menu + action button
    if ((laddderMenuOption == COMMANDTAP) && instrActionState[i] && interfaceEvent.debounced())
    {
      //calculate if the tap will be saved on this step or into next
      int8_t updateDisplayTapStep;
      if (micros() < (u_lastTick + (u_tickInterval >> 1))) //if we are still in the same step
      {
        //DRAGGED STEP - tapped after trigger time
        //updateDisplayTapStep = stepCount-1;
        updateDisplayTapStep = ((stepCount - 1) < 0) ? (MAXSTEPS - 1) : (stepCount - 1);
        //if this step is empty, play sound
        if (!deck[thisDeck]->pattern->isThisStepActive(i, deck[thisDeck]->deckPatterns[i]->getValue(), updateDisplayTapStep))
        {
          wTrig.trackPlayPoly(deck[thisDeck]->deckSamples[i]->getValue() + 1); //send a wtrig async play
        }
      }
      else
      {
        //RUSHED STEP - tapped before trigger time
        //updateDisplayTapStep = ((stepCount) >= MAXSTEPS) ? 0 : (stepCount);
        updateDisplayTapStep = stepCount;
        //tap saved to next step
        // int8_t nextStep = stepCount;
        // nextStep++;
        // if (nextStep >= MAXSTEPS)
        //   nextStep = 0;
        // updateDisplayTapStep = nextStep;
        wTrig.trackPlayPoly(deck[thisDeck]->deckSamples[i]->getValue() + 1); //send a wtrig async play
        deck[thisDeck]->tappedStep[i] = true;
      }
      deck[thisDeck]->pattern->tapStep(i, deck[thisDeck]->deckPatterns[i]->getValue(), updateDisplayTapStep);
      updateDisplayTapInstrumentPattern = i;
      interfaceEvent.debounce(250);
    }

    //if any tap should be rollbacked - ladder menu + action button
    if ((laddderMenuOption == COMMANDUND) && instrActionState[i] && deck[thisDeck]->pattern->isThisCustomPattern(i) && interfaceEvent.debounced())
    {
      //if there was any rollback available
      if (deck[thisDeck]->pattern->undoAvailable(i))
      {
        deck[thisDeck]->pattern->rollbackUndoStep(i, deck[thisDeck]->deckPatterns[i]->getValue()); //rollback the last saved undo
        displayShowInstrPattern(i, RAM);                                                           //update display with new inserted step
        updateDisplay = true;
        updateDisplayRollbackInstrumentTap = i;
        interfaceEvent.debounce(200);
      }

      //if any pattern should be erased - ladder menu + action button
      else if ((laddderMenuOption == COMMANDERS) && instrActionState[i] && interfaceEvent.debounced())
      {
        //erase pattern IF possible
        deck[thisDeck]->pattern->eraseInstrumentPattern(i, deck[thisDeck]->deckPatterns[i]->getValue());
        interfaceEvent.debounce(1000);
        updateDisplayEraseInstrumentPattern = i;
      }

      //if any pattern should be permanently muted - ladder menu + action button
      else if ((laddderMenuOption == COMMANDSIL) && instrActionState[i] && interfaceEvent.debounced())
      {
        deck[thisDeck]->permanentMute[i] = deck[thisDeck]->permanentMute[i];
        interfaceEvent.debounce(1000);
      }
    }
  }
  //long time readings from action ladder menu
  if (laddderMenuReadInterval.debounced())
  {
    laddderMenuReadInterval.debounce();
    ladderMenu.readSmoothed();
    laddderMenuOption = ladderMenu.getSmoothedMenuIndex() + 1;
  }

  //if it is time to read one new melody parameter
  if (melodieUpdate.debounced())
  {
    melodieUpdate.debounce();
    int16_t read;
    //add 8bit resolution parameter to a filter
    read = paramFilter[queuedMelodyParameter]->add(analogRead(queuedMelodyParameter) >> 4);
    boolean newUpdates;
    newUpdates = melody->updateParameters(queuedMelodyParameter, read);
    if (newUpdates)
    {
      digitalWrite(TRIGOUTPATTERNPIN1, HIGH);
      info.debounce();
      infoState = true;
    }
    queuedMelodyParameter++;
    if (queuedMelodyParameter >= MAXMELODYPARMS)
      queuedMelodyParameter = 0;
  }
  if (infoState && info.debounced())
  {
    infoState = false;
    digitalWrite(TRIGOUTPATTERNPIN1, LOW);
  }

  // //clock source switch from external to internal
  // if (externalClockSource && switchBackToInternalClock.debounced())
  // {
  //   externalClockSource = false;
  //   Timer3.attachInterrupt(ISRfireTimer3);
  //   Timer3.start(u_bpm);
  // }
}

void printDeck(boolean _deck)
{
  Serial.println(_deck);
  for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++)
  {
    Serial.print(deck[_deck]->deckSamples[instr]->getValue());
    Serial.print(",");
    Serial.print(deck[_deck]->deckPatterns[instr]->getValue());
    Serial.print(",");
    Serial.print(deck[_deck]->permanentMute[instr]);
    Serial.print(",");
    Serial.print(deck[_deck]->gateLenghtSize[instr]);
    Serial.println("");
  }
  Serial.println(".");
}
