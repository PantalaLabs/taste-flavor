/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create melodies
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "tf_Defines.h" //global taste and flavor defines
#include "Pinout.h"     //pinout defines
#include <Arduino.h>
#include <Rotary.h>
#include <EventDebounce.h>
#include <PantalaDefines.h>
#include <DueTimer.h>
#include <AnalogInput.h>
#include <Trigger.h>
#include "Mood.h"

//decks
Mood *mood[2];
boolean thisDeck = 0;

#if DO_WT == true
#include <wavTrigger.h>
wavTrigger wTrig;
#endif

#if DO_SD == true
#include "SdComm.h"
SdComm *sdc;
#endif

//display
#define DISPLAYUPDATETIME 35000 //microsseconds to update display

//adafruit
#include <Adafruit_SSD1306.h>
#define OLED_RESET 43 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, OLED_RESET);

// //CV sequencer
#include "CVSequence.h"
CVSequence *cvseq;
// #include "Melody.h"
// Melody *cvseq;

//ladder menu
#define MAXPARAMETERS 9
#define COMMANDSMP 0
#define COMMANDGHO 1
#define COMMANDTAP 2
#define COMMANDUND 3
#define COMMANDSIL 4
#define COMMANDERS 5
#define COMMANDGTL 6
#define COMMANDLAT 7
#define COMMANDSOL 8

AnalogInput ladderMenu(G_LADDERMENUPIN);
uint8_t laddderMenuOption;
int laddderMenuSeparators[MAXPARAMETERS + 1] = {30, 140, 290, 490, 620, 750, 840, 900, 980, G_MAX10BIT};
EventDebounce laddderMenuReadInterval(100);

//time related and bpm
volatile uint32_t u_lastTick;                   //last time tick was called
volatile uint32_t u_tickInterval = 1000000;     //tick interval
volatile uint32_t safeZoneEndTime;              //safe time zone
boolean updateDisplayUpdateLatencyComp = false; //flag to update latency compensation
int32_t u_LatencyComp = 1100;                   //default latency compensation keep the 100 microsseconds there
int32_t u_LatencyCompStep = 1000;               //latency compensation amount update step
#define u_LatencyCompLimit 20000                //latency compensation + and - limit
uint16_t bpm = 125;                             //actual bpm in bpm
uint32_t u_bpm = 0;                             //actual bpm value in us

//encoders buttons : mood, cross, instruments
uint8_t encoderButtonPins[MAXENCODERS] = {ENCBUTPINMOOD, ENCBUTPINCROSS, ENCBUTPININSTR1, ENCBUTPININSTR2, ENCBUTPININSTR3, ENCBUTPININSTR4, ENCBUTPININSTR5, ENCBUTPININSTR6};
boolean encoderButtonState[MAXENCODERS] = {0, 0, 0, 0, 0, 0, 0, 0};

//action pins
uint8_t instrActionPins[G_MAXINSTRUMENTS] = {ACTIONPININSTR1, ACTIONPININSTR2, ACTIONPININSTR3, ACTIONPININSTR4, ACTIONPININSTR5, ACTIONPININSTR6}; //pins
boolean instrActionState[G_MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};

EventDebounce instrActionDebounce1(200);
EventDebounce instrActionDebounce2(200);
EventDebounce instrActionDebounce3(200);
EventDebounce instrActionDebounce4(200);
EventDebounce instrActionDebounce5(200);
EventDebounce instrActionDebounce6(200);
EventDebounce instrActionDebounceArray[G_MAXINSTRUMENTS] = {instrActionDebounce1, instrActionDebounce2, instrActionDebounce3, instrActionDebounce4, instrActionDebounce5, instrActionDebounce6};

//triggers
volatile int8_t gateLenghtCounter = 0;
uint8_t triggerPins[G_MAXINSTRUMENTS] = {TRIGOUTPIN1, TRIGOUTPIN2, TRIGOUTPIN3, TRIGOUTPIN4, TRIGOUTPIN5, TRIGOUTPIN6};

Trigger trigOutPattern(TRIGOUTPATTERNPIN);
Trigger parameterChange(PARAMETERCHANGE);

EventDebounce interfaceEvent(200); //min time in ms to accept another interface event
EventDebounce cvseqUpdate(70);     //min time in ms read new cv sequence parameter

Rotary moodEncoderPot(ENCPINAMOOD, ENCPINBMOOD);
Rotary crossEncoderPot(ENCPINACROSS, ENCPINBCROSS);
Rotary instr1encoder(ENCPINAINSTR1, ENCPINBINSTR1);
Rotary instr2encoder(ENCPINAINSTR2, ENCPINBINSTR2);
Rotary instr3encoder(ENCPINAINSTR3, ENCPINBINSTR3);
Rotary instr4encoder(ENCPINAINSTR4, ENCPINBINSTR4);
Rotary instr5encoder(ENCPINAINSTR5, ENCPINBINSTR5);
Rotary instr6encoder(ENCPINAINSTR6, ENCPINBINSTR6);
Rotary *encoders[MAXENCODERS] = {&moodEncoderPot, &crossEncoderPot, &instr1encoder, &instr2encoder, &instr3encoder, &instr4encoder, &instr5encoder, &instr6encoder};

int16_t selectedMood = 0;                //actual selected mood
int16_t lastSelectedMood = 255;          //prevents to execute 2 times the same action
int16_t previousMoodname = 0;            //previous mood name
int8_t lastCrossBarGraphValue = 127;     //0 to G_MAXINSTRUMENTS possible values
boolean flagUD_browseThisMood = false;   //schedule some display update
boolean flagUD_newMoodSelected = false;  //schedule some display update
boolean flagUD_newBpmValue = false;      //update bpm
int8_t flagUD_newCrossfaderPosition = 0; //schedule some display update
int8_t flagUD_newPlayingPattern = -1;    //update only one instrument pattern
int8_t flagUD_newPlayingSample = -1;     //update sample on rasp pi
int8_t flagUD_newGateLenght = -1;        //update gate lenght on right upper corner
int8_t flagUD_eraseThisPattern = -1;     //erase selected pattern
int8_t flagUD_tapNewStep = -1;           //tap new step
int8_t flagUD_rollbackTappedStep = -1;   //undo tapped step
boolean flagUD_newClockSource = false;   //changes clock source

boolean defaultDisplayNotActiveYet = true;
boolean internalClockSource = true; //0=internal , 1=external

EventDebounce switchBackToInternalClock(3000);

int8_t soloInstrument;

//sequencer
volatile int8_t stepCount = 0;
int8_t queuedRotaryEncoder = 0;

//mood
uint16_t importedMoodsFromSd = 0;
char *moodKitName[G_MAXMEMORYMOODS] = {
    "Empty",
    "P.Labs-Empty Room",
    "P.Labs-Choke",
    "P.Labs-April23",
    "Carlos Pires-Drama",
    "P.Labs-Fat Cortex",
    "P.Labs-Straight Lane",
    "P.Labs-Chained",
    "P.Labs-EasyBreak",
    "Edu M-Crispy Mood",
    "P.Labs-Dbl Bass",
    "P.Labs-Melotech",
    "Fab.Pecanha-Groove1",
    "Fab.Pecanha-Groove2",
    "Fab.Pecanha-Groove3",
    "Fab.Pecanha-Groove4",
    "Fab.Pecanha-Groove5",
    "Fab.Pecanha-Groove6",
    "Fab.Pecanha-Groove7",
    "Fab.Pecanha-Groove8",
};
//{pattern id, pattern id, pattern id, pattern id, pattern id, pattern id, absolute volume reduction}
//reserved SWITCH = 0
//reserved MUTE = 1
uint16_t moodKitData[G_MAXMEMORYMOODS][G_MAXINSTRUMENTS] = {
    {1, 1, 1, 1, 1, 1},    //reserved MUTE = 1
    {2, 2, 2, 2, 2, 2},    //P.Labs-Empty Room
    {2, 3, 3, 4, 3, 1},    //P.Labs-Choke
    {2, 3, 4, 2, 3, 1},    //P.Labs-April23
    {2, 5, 5, 2, 5, 4},    //Carlos Pires-Drama
    {2, 6, 5, 6, 6, 1},    //P.Labs-Fat Cortex
    {2, 4, 6, 4, 7, 5},    //P.Labs-Straight Lane
    {2, 3, 7, 2, 3, 2},    //P.Labs-Chained
    {3, 7, 8, 4, 4, 6},    //P.Labs-EasyBreak
    {2, 8, 9, 5, 8, 7},    //Edu M-Crispy Mood
    {3, 9, 10, 4, 6, 3},   //Plabs-Dbl Bass
    {4, 4, 3, 5, 1, 1},    //Plabs-Melotech
    {2, 10, 3, 4, 2, 8},   //Fab.Pecanha-Groove1
    {2, 11, 3, 4, 9, 9},   //Fab.Pecanha-Groove2
    {2, 8, 3, 4, 10, 10},  //Fab.Pecanha-Groove3
    {2, 11, 3, 4, 9, 11},  //Fab.Pecanha-Groove4
    {2, 12, 11, 2, 3, 12}, //Fab.Pecanha-Groove5
    {2, 8, 3, 7, 3, 6},    //Fab.Pecanha-Groove6
    {2, 13, 3, 4, 11, 9},  //Fab.Pecanha-Groove7
    {2, 14, 3, 10, 3, 13}  //Fab.Pecanha-Groove8
};

//decks
int8_t crossfader = 0;
int8_t lastCrossfadedValue = G_MAXINSTRUMENTS;

#include "displayProcs.h"

void setup()
{

//serial monitor
#if DO_SERIAL == true
  Serial.begin(9600);
  Serial.println("Debugging..");
#endif

  analogWriteResolution(12);
  analogReadResolution(10);
  pinMode(DAC0, OUTPUT);

  pinMode(TRIGGERINPIN, INPUT_PULLUP);

  pinMode(RESETINPIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RESETINPIN), ISRresetSeq, FALLING);

  ladderMenu.setMenu(laddderMenuSeparators, MAXPARAMETERS + 1);

  //setup display Address 0x3D for 128x64
  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS))
  {
#if DO_SERIAL == true
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed
#endif
  }
  else
  {
#if DO_SERIAL == true
    Serial.println(F("Display OK!"));
#endif
  }
  delay(100);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE); // white text
  display.cp437(true);
  display.setCursor(0, 0);
  display.display();

  //start all encoders and it´s buttons
  for (uint8_t i = 0; i < MAXENCODERS; i++)
  {
    encoders[i]->begin();
    pinMode(encoderButtonPins[i], INPUT_PULLUP);
  }

  //start all Action buttons
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    pinMode(instrActionPins[i], INPUT_PULLUP);

  //start all trigger out pins
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    pinMode(triggerPins[i], OUTPUT);
    digitalWrite(triggerPins[i], LOW);
  }

  //trigger out TEST
  trigOutPattern.start();
  parameterChange.start();
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    digitalWrite(triggerPins[i], HIGH);
    analogWrite(DAC0, i * 512);
    delay(100);
  }
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    digitalWrite(triggerPins[i], LOW);
    analogWrite(DAC0, (G_MAXINSTRUMENTS - i) * 512);
    delay(100);
  }
  analogWrite(DAC0, 0);
  trigOutPattern.end();
  parameterChange.end();

  //if mood encoder button pressed , do a full DAC voltage test
  while (!digitalRead(ENCBUTPINMOOD))
  {
    analogWrite(DAC0, 4095);
    delay(5000);
    analogWrite(DAC0, 0);
    delay(5000);
  }

  //if crossfader button pressed  starts ladder menu
  if (!digitalRead(ENCBUTPINCROSS))
  {
    uint16_t ladderRead;
#if DO_SERIAL == true
    Serial.println(F("Ladder menu"));
#endif
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE); // white text
    display.cp437(true);
    display.setCursor(0, 0);
    while (!digitalRead(ENCBUTPINCROSS))
    {
      ladderRead = analogRead(G_LADDERMENUPIN);
#if DO_SERIAL == true
      Serial.println(ladderRead);
#endif
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print(ladderRead);
      display.display();
    }
  }

  //if instrument 1 button pressed , show all analog read
  while (!digitalRead(ENCBUTPININSTR1))
  {
    for (uint8_t i = 0; i < 8; i++)
    {
      Serial.print(analogRead(i + 1));
      Serial.print(",");
    }
    Serial.println(".");
  }

  //if instrument 2 button pressed , show all trigger leds
  if (!digitalRead(ENCBUTPININSTR2))
  {
    trigOutPattern.start();
    parameterChange.start();
    for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
      digitalWrite(triggerPins[i], HIGH);
    delay(5000);
    while (digitalRead(ENCBUTPININSTR1))
    {
      //if instrument 2 button pressed again  , exit test
    }
    trigOutPattern.end();
    parameterChange.end();
    for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
      digitalWrite(triggerPins[i], LOW);
  }

  cvseq = new CVSequence();
  //cvseq = new Melody();

  //start decks
  mood[0] = new Mood(G_INTERNALMOODS);
  mood[1] = new Mood(G_INTERNALMOODS);

  //legacy : convert old boolean arrays to new byte block arrays
  // mood[0]->instruments[0]->convertBooleanToByte1();
  // mood[0]->instruments[1]->convertBooleanToByte1();
  // mood[0]->instruments[2]->convertBooleanToByte1();
  // mood[0]->instruments[3]->convertBooleanToByte1();
  // mood[0]->instruments[4]->convertBooleanToByte1();
  // mood[0]->instruments[5]->convertBooleanToByte1();
  //mood[0]->instruments[0]->legacyEuclidBooleanToByte();
  // calcE(2, 3);
  // printE();
  // calcE(2, 4);
  // printE();
  // calcE(2, 5);
  // printE();
  // calcE(3, 5);
  // printE();
  // calcE(3, 7);
  // printE();
  // calcE(3, 8);
  // printE();
  // calcE(4, 7);
  // printE();
  // calcE(4, 9);
  // printE();
  // calcE(4, 11);
  // printE();
  // calcE(5, 6);
  // printE();
  // calcE(5, 7);
  // printE();
  // calcE(5, 8);
  // printE();
  // calcE(5, 9);
  // printE();
  // calcE(5, 11);
  // printE();
  // calcE(5, 12);
  // printE();
  // calcE(5, 16);
  // printE();
  // calcE(7, 8);
  // printE();
  // calcE(7, 12);
  // printE();
  // calcE(7, 16);
  // printE();
  // calcE(9, 16);
  // printE();
  // calcE(11, 24);
  // printE();
  // calcE(13, 24);
  // printE();

  //import moods from SD card
#if DO_SD == true
  sdc = new SdComm(SD_CS, true);
  sdc->importMoods(moodKitName, moodKitData, G_INTERNALMOODS);
  importedMoodsFromSd = sdc->getimportedRecords();
  //update decks to recognize new moods
  mood[0]->changeMaxMoods(G_INTERNALMOODS + importedMoodsFromSd);
  mood[1]->changeMaxMoods(G_INTERNALMOODS + importedMoodsFromSd);
#endif

#if DO_WT == true
  // WAV Trigger startup at 57600
  delay(1000);
  wTrig.start();
  delay(50);
  wTrig.stopAllTracks();
  wTrig.masterGain(-9);
  wTrig.samplerateOffset(0);
  wTrig.setReporting(true);
  delay(100);
#endif

  //if instrument 3 button pressed , display SDA/SCL test
  if (!digitalRead(ENCBUTPININSTR3))
  {
#if DO_SERIAL == true
    Serial.println(F("Display Test"));
#endif
    digitalWrite(TRIGOUTPATTERNPIN, HIGH);
    delay(3000);
    while (digitalRead(ENCBUTPININSTR3))
    {
      display.clearDisplay();
      display.setTextSize(1);      // Normal 1:1 pixel scale
      display.setTextColor(WHITE); // white text
      display.cp437(true);
      display.setCursor(0, 0);
      display.println(F("Pantala Labs"));
      display.println(F("Taste & Flavor"));
      display.println(F(""));
      //if instrument 3 button pressed again  , exit test
    }
  }

  //if instrument 4 button pressed , display DAC test
  if (!digitalRead(ENCBUTPININSTR4))
  {
#if DO_SERIAL == true
    Serial.println(F("DAC Test"));
#endif
    digitalWrite(TRIGOUTPATTERNPIN, HIGH);
    delay(3000);
    while (digitalRead(ENCBUTPININSTR4))
    {
      analogWrite(DAC0, 0);
      delay(4000);
      analogWrite(DAC0, G_MAX12BIT);
      delay(4000);
      //if instrument 4 button pressed again  , exit test
    }
  }

  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // white text
  display.cp437(true);
  display.setCursor(0, 0);
  display.println(F("Pantala Labs"));
  display.println(F("Taste & Flavor"));
  display.println(F(""));

#if DO_WT == true
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
#endif
  display.display();
  delay(3000);
  displayWelcome();
  display.display();

  //internal clock
  u_bpm = bpm2micros4ppqn(bpm);
  Timer3.attachInterrupt(ISRbaseTimer);
  Timer3.start(u_bpm);
  Timer4.attachInterrupt(ISRfireTimer4);
  Timer5.attachInterrupt(ISRendTriggers);

#if DO_SERIAL == true
  Serial.println("Setup ends.");
  delay(50);
#endif
}

void debugBeep(uint8_t _beep)
{
  delay(1000);
  for (uint8_t i = 0; i < _beep; i++)
  {
    parameterChange.start();
    delay(250);
    parameterChange.end();
    delay(250);
  }
  delay(1000);
}

//base clock / external clock
void ISRbaseTimer()
{
  noInterrupts();
  if (internalClockSource)
    Timer3.stop();
  u_tickInterval = micros() - u_lastTick;
  u_lastTick = micros();
  if (u_LatencyComp >= 0)
    Timer4.start(u_LatencyComp);
  else
    Timer4.start(u_tickInterval + u_LatencyComp);
  if (internalClockSource)
    Timer3.start(bpm2micros4ppqn(bpm));
  interrupts();
}

//shifted clock and everything to step sequencer related
void ISRfireTimer4()
{
  noInterrupts();
  Timer5.start(G_DEFAULTGATELENGHT); //start 5ms first trigger timer
  Timer4.stop();                     //stop timer shift

  for (int8_t instr = 0; instr < G_MAXINSTRUMENTS; instr++) //for each instrument
  {
    boolean targetDeck;
    targetDeck = crossfadedDeck(instr);

#if DO_WT == true
    //play a sample if :
    //if not solo + pattern ready + it wasnt manually muted
    //it was manually actioned together with laddermenu
    //if (mood[targetDeck]->instruments[instr]->playThisStep(stepCount) && (!instrActionState[instr] || (instrActionState[instr] && (laddderMenuOption < MAXPARAMETERS))))
    if ((soloInstrument == -1) && (mood[targetDeck]->instruments[instr]->playThisStep(stepCount) && !instrActionState[instr]))
    {
      wTrig.trackGain(mood[targetDeck]->samples[instr]->getValue(), -6);
      wTrig.trackLoad(mood[targetDeck]->samples[instr]->getValue());
    }
    //play only solo instrument
    else if ((soloInstrument == instr) && (mood[targetDeck]->instruments[instr]->playThisStep(stepCount) && !instrActionState[instr]))
    {
      wTrig.trackGain(mood[targetDeck]->samples[instr]->getValue(), -6);
      wTrig.trackLoad(mood[targetDeck]->samples[instr]->getValue());
    }
#endif
    //fire instrument trigger even if sample was silenced
    if (mood[targetDeck]->instruments[instr]->playThisTrigger(stepCount) && !instrActionState[instr])
      digitalWrite(triggerPins[instr], HIGH);

    mood[targetDeck]->instruments[instr]->tappedStep = false; //reset any tapped step
  }
#if DO_WT == true
  wTrig.resumeAllInSync();
#endif
  interrupts();
  analogWrite(DAC0, cvseq->getNote());
  //analogWrite(DAC1, cvseq->getSubNote());
  gateLenghtCounter = 0;
  if ((stepCount == 0) || (stepCount == 60) || (stepCount == 56))
    trigOutPattern.start();
  stepCount++;
  if (stepCount >= G_MAXSTEPS)
    ISRresetSeq();
  safeZoneEndTime = (micros() + u_tickInterval - DISPLAYUPDATETIME);
}

void ISRresetSeq()
{
  stepCount = 0;
  cvseq->resetStepCounter();
}

//close all trigger
//  _ ______ _______ _______ _______
//_| |      |       |       |       |______
void ISRendTriggers()
{
  //compare all triggers times
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    if (mood[crossfadedDeck(i)]->instruments[i]->gateLenghtSize <= gateLenghtCounter)
      digitalWrite(triggerPins[i], LOW);
  }
  //prepare to the next gate verification
  gateLenghtCounter++;
  if (gateLenghtCounter > G_MAXGATELENGHTS) //if all step steps ended
  {
    Timer5.stop(); //stops trigger timer
    gateLenghtCounter = 0;
  }
  else
  {
    Timer5.stop();                      //stops trigger timer
    Timer5.start(G_EXTENDEDGATELENGHT); //start another 80ms trigger gap timer
  }
}

//allows to change samples only upo to after 2/3 of the tick interval
//to avoid to change sample the same time it was triggered
boolean safeZone()
{
  return (micros() < safeZoneEndTime);
}

//return if this instrument belongs to this or not to this deck
boolean crossfadedDeck(uint8_t _instr)
{
  return (_instr < crossfader) ? thisDeck : !thisDeck;
}

//read queued encoder status
void processRotaryEncoder()
{
  queuedRotaryEncoder++;
  if (queuedRotaryEncoder == MAXENCODERS)
    queuedRotaryEncoder = 0;

  unsigned char result = encoders[queuedRotaryEncoder]->process();
  if ((result == 16) || (result == 32))
  {
    int8_t encoderChange;
    encoderChange = (result == 16) ? 1 : -1;
    if (queuedRotaryEncoder == MOODENCODER)
    {
      selectedMood += encoderChange;
      selectedMood = constrain(selectedMood, 0, G_INTERNALMOODS - 1);
      if (lastSelectedMood != selectedMood)
      {
        flagUD_browseThisMood = true;
        lastSelectedMood = selectedMood;
      }
    }
    else if (queuedRotaryEncoder == CROSSENCODER)
    {
      //if cross button AND cross button are pressed and cross rotate change timer shift
      if (laddderMenuOption == COMMANDLAT)
      {
        u_LatencyComp += encoderChange * u_LatencyCompStep;
        u_LatencyComp = constrain(u_LatencyComp, -u_LatencyCompLimit, u_LatencyCompLimit);
        updateDisplayUpdateLatencyComp = true;
      }
      //if cross button is pressed and cross rotate CHANGE BPM
      if (onlyOneEncoderButtonIsPressed(ENCBUTCROSS) && interfaceEvent.debounced())
      {
        flagUD_newBpmValue = true;
        bpm += encoderChange;
        bpm = constrain(bpm, G_MINBPM, G_MAXBPM);
        u_bpm = bpm2micros4ppqn(bpm);
        interfaceEvent.debounce(50); //block any other interface event
      }
      else
      { //or else , cross change
        crossfader += encoderChange;
        crossfader = constrain(crossfader, 0, G_MAXINSTRUMENTS);
        flagUD_newCrossfaderPosition = encoderChange;
      }
    }
    else
    {
      //all other instrument encoders
      uint8_t _instrum = queuedRotaryEncoder - 2; //discard mood and morph indexes
      //sample change
      if (laddderMenuOption == COMMANDSMP)
      {
        if (encoderChange == -1)
          mood[thisDeck]->samples[_instrum]->safeChange(-G_MAXINSTRUMENTS);
        else
          mood[thisDeck]->samples[_instrum]->safeChange(G_MAXINSTRUMENTS);
        flagUD_newPlayingSample = _instrum;
      }
      //gate lenght change
      else if (laddderMenuOption == COMMANDGTL)
      {
        mood[thisDeck]->instruments[_instrum]->changeGateLenghSize(encoderChange);
        flagUD_newGateLenght = _instrum;
      }
      //pattern change
      else if (noOneEncoderButtonIsPressed())
      {
        if (encoderChange == -1)
          mood[thisDeck]->instruments[_instrum]->patternIndex->reward();
        else
          mood[thisDeck]->instruments[_instrum]->patternIndex->advance();
        flagUD_newPlayingPattern = _instrum; //flags to update this instrument on display
      }
    }
  }
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
  if (!encoderButtonState[_target]) //if asked button is not pressed
    return false;
  for (uint8_t i = 0; i < MAXENCODERS; i++)      //search all encoder buttons
    if ((i != _target) && encoderButtonState[i]) //encoder button is not the asked one and it is pressed , so there are 2 encoder buttons pressed
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
  //read queued encoders
  processRotaryEncoder();

  soloInstrument = mood[thisDeck]->getSoloInstrument();

  //ASYNC updates ==============================================================
  //flags if any display update will be necessary in this cycle
  boolean updateDisplay = false;
  //mood browse
  if (flagUD_browseThisMood)
  {
    checkDefaultDisplay();
    displayShowBrowsedMood();
    flagUD_browseThisMood = false;
    updateDisplay = true;
  }
  //right upper corner with bpm value
  else if (flagUD_newBpmValue)
  {
    flagUD_newBpmValue = false;
    displayShowCornerInfo(2, bpm);
    updateDisplay = true;
  }
  //cross bar changes inside sample update window
  //this could be took off if using Tsunami
  else if ((flagUD_newCrossfaderPosition != 0) && safeZone())
  {
    checkDefaultDisplay();
    displayShowCrossBar(crossfader);
    flagUD_newCrossfaderPosition = 0;
    updateDisplay = true;
  }
  //copy selected mood to new deck inside sample update window
  else if (flagUD_newMoodSelected && safeZone())
  {
    checkDefaultDisplay();
    //before load new mood, prepare current deck to leave it the same way user customized
    for (uint8_t instr = 0; instr < G_MAXINSTRUMENTS; instr++)
    {
      //if this instrument belongs to the other deck
      if (crossfadedDeck(instr) == !thisDeck)
      {
        //if this instrument was left behind on the other deck , save it on new
        if (crossfader <= lastCrossfadedValue)
        {
          mood[thisDeck]->cueXfadedInstrument(
              instr,
              mood[!thisDeck]->samples[instr]->getValue(),
              mood[!thisDeck]->instruments[instr]->patternIndex->getValue(),
              mood[!thisDeck]->instruments[instr]->permanentMute,
              mood[!thisDeck]->instruments[instr]->gateLenghtSize);
        }
        else //or discard
        {
          mood[thisDeck]->discardNotXfadedInstrument(instr);
        }
      }
      lastCrossfadedValue = crossfader;
    }
    //switch decks
    crossfader = 0;
    thisDeck = !thisDeck;
    mood[thisDeck]->cue(selectedMood,
                        moodKitData[selectedMood][0], //patterns
                        moodKitData[selectedMood][1], //
                        moodKitData[selectedMood][2], //
                        moodKitData[selectedMood][3], //
                        moodKitData[selectedMood][4], //
                        moodKitData[selectedMood][5]  //
    );
    displayUpdateLineArea(1, moodKitName[previousMoodname]);
    displayUpdateLineArea(2, moodKitName[selectedMood]);
    displayShowCrossBar(-1);
    previousMoodname = selectedMood;
    flagUD_newMoodSelected = false;
    updateDisplay = true;
  }
  //if only one intrument pattern changed
  else if (flagUD_newPlayingPattern != -1)
  {
    displayShowInstrPattern(flagUD_newPlayingPattern, RAM);
    displayUpdateLineArea(3, "Custom");
    displayShowCornerInfo(0, mood[thisDeck]->instruments[flagUD_newPlayingPattern]->patternIndex->getValue());
    updateDisplay = true;
    flagUD_newPlayingPattern = -1;
  }
  //if sampler changed and there is available time to update screen
  else if ((flagUD_newPlayingSample != -1) && safeZone())
  {
    displayUpdateLineArea(3, moodKitName[mood[thisDeck]->samples[flagUD_newPlayingSample]->getValue() / 6]);
    displayShowCornerInfo(1, mood[thisDeck]->samples[flagUD_newPlayingSample]->getValue());
    updateDisplay = true;
    flagUD_newPlayingSample = -1;
  }
  //if latency compensation changed
  else if (updateDisplayUpdateLatencyComp)
  {
    displayShowCornerInfo(3, (u_LatencyComp - 100) / 1000);
    updateDisplay = true;
    updateDisplayUpdateLatencyComp = false;
  }
  //if gate lenght changed
  else if (flagUD_newGateLenght != -1)
  {
    displayShowCornerInfo(4, flagUD_newGateLenght);
    updateDisplay = true;
    flagUD_newGateLenght = -1;
  }
  //if erase pattern
  else if (flagUD_eraseThisPattern != -1)
  {
    //clear pattern from display
    displayEraseInstrumentPattern(flagUD_eraseThisPattern);
    updateDisplay = true;
    flagUD_eraseThisPattern = -1;
  }
  //if step was tapped
  else if (flagUD_tapNewStep != -1)
  {
    //update display with new inserted step
    displayShowInstrPattern(flagUD_tapNewStep, RAM);
    updateDisplay = true;
    flagUD_tapNewStep = -1;
  }
  //rolback tap
  else if (flagUD_rollbackTappedStep != -1)
  {
    //update display with removed step
    displayShowInstrPattern(flagUD_rollbackTappedStep, RAM);
    updateDisplay = true;
    flagUD_rollbackTappedStep = -1;
  }
  //change clock source
  else if (flagUD_newClockSource)
  {
    flagUD_newClockSource = false;
    if (internalClockSource)
    {
      detachInterrupt(digitalPinToInterrupt(TRIGGERINPIN));
      Timer3.attachInterrupt(ISRbaseTimer);
      Timer3.start(u_bpm);
    }
    else
    {
      Timer3.stop();
      Timer3.detachInterrupt();
      attachInterrupt(digitalPinToInterrupt(TRIGGERINPIN), ISRbaseTimer, FALLING);
    }
    displayClockSource();
    updateDisplay = true;
  }

  //if any display update
  if (updateDisplay)
    display.display();

  //SYNCd updates ==============================================================
  if (interfaceEvent.debounced())
  {
    //read all encoders buttons (invert boolean state to make it easyer for compararison)
    for (int8_t i = 0; i < MAXENCODERS; i++)
    {
      encoderButtonState[i] = !digitalRead(encoderButtonPins[i]);
      //for any any INSTRUMENT encoder button
      if (i >= 2)
      {
        //if it was pressed with menu command silence, silence currently UNloaded deck
        if (laddderMenuOption == COMMANDSIL && encoderButtonState[i])
        {
          mood[!thisDeck]->instruments[i - 2]->permanentMute = !mood[!thisDeck]->instruments[i - 2]->permanentMute;
          interfaceEvent.debounce(300);
        }
      }
    }
    //change clock source
    if (encoderButtonState[6] && encoderButtonState[7])
    {
      flagUD_newClockSource = true;
      internalClockSource = !internalClockSource;
      interfaceEvent.debounce(1000);
    }
  }

  //if new mood was selected....copy reference tables or create new mood in the cross area
  if (onlyOneEncoderButtonIsPressed(ENCBUTMOOD) && interfaceEvent.debounced())
  {
    interfaceEvent.debounce(1000); //block any other interface event
    flagUD_newMoodSelected = true;
  }

  //all action (mute) buttons ===============================================================
  for (int8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    //read action instrument button (invert boolean state to make easy comparation)
    instrActionState[i] = !digitalRead(instrActionPins[i]);
    //if interface available and action button pressed
    if (instrActionDebounceArray[i].debounced() && instrActionState[i])
    {
      boolean makeDebounce = false;
      //selected ladder menu option
      switch (laddderMenuOption)
      {
      case COMMANDTAP:
        //compute if the tap will be saved on this step or into next
        int8_t updateDisplayTapStep;
        if (micros() < (u_lastTick + (u_tickInterval >> 1))) //if we are still in the same step
        {
          //DRAGGED STEP - tapped after trigger time
          //update display tapping previous step
          updateDisplayTapStep = ((stepCount - 1) < 0) ? (G_MAXSTEPS - 1) : (stepCount - 1);
#if DO_WT == true
          //if this step is empty, play sound
          //if this is a valid step for this deck and this instrument isnt silenced
          if (!mood[thisDeck]->instruments[i]->getStep(updateDisplayTapStep) && !mood[thisDeck]->instruments[i]->permanentMute)
            wTrig.trackPlayPoly(mood[thisDeck]->samples[i]->getValue() + 1); //send a wtrig async play
#endif
        }
        else
        {
          //RUSHED STEP - tapped before trigger time
          //update display tapping current step
          updateDisplayTapStep = stepCount;
#if DO_WT == true
          //if this instrument isnt silenced
          if (!mood[thisDeck]->instruments[i]->permanentMute)
            wTrig.trackPlayPoly(mood[thisDeck]->samples[i]->getValue() + 1); //send a wtrig async play
#endif
          mood[thisDeck]->instruments[i]->tappedStep = true;
        }
        mood[thisDeck]->instruments[i]->tapStep(updateDisplayTapStep);
        flagUD_tapNewStep = i;
        makeDebounce = true;
        break;
      case COMMANDUND:
        //if this pattern is a custom one and there was any rollback available
        if (mood[thisDeck]->instruments[i]->undoLastTappedStep())
        {
          displayShowInstrPattern(i, RAM); //update display with new inserted step
          updateDisplay = true;
          flagUD_rollbackTappedStep = i;
        }
        makeDebounce = true;
        break;
      case COMMANDERS:
        //erase pattern if possible
        mood[thisDeck]->instruments[i]->eraseInstrumentPattern();
        flagUD_eraseThisPattern = i;
        makeDebounce = true;
        break;
      case COMMANDSIL:
        //silence only currently loaded deck
        mood[thisDeck]->instruments[i]->permanentMute = !mood[thisDeck]->instruments[i]->permanentMute;
        makeDebounce = true;
        break;
      case COMMANDSOL:
        //solo an instrument
        mood[thisDeck]->setSoloInstrument(i);
        makeDebounce = true;
        break;
      }
      if (makeDebounce)
        instrActionDebounceArray[i].debounce();
    }
  }
  //long interval between readings from action ladder menu
  if (laddderMenuReadInterval.hitAndRun())
    laddderMenuOption = ladderMenu.getRawMenuIndex();

  //if it is time to read one new cv sequence parameter input
  if (cvseqUpdate.hitAndRun() && cvseq->readNewParameter())
    parameterChange.start();

  trigOutPattern.compute();
  parameterChange.compute();
}
