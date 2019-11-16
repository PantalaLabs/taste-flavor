/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create melodies
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "tf_Defines.h" //global taste and flavor defines

#include <Arduino.h>
#include <PantalaDefines.h>
#include <EventDebounce.h>
#include <Rotary.h>
#include <DueTimer.h>
#include <AnalogInput.h>
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
#define OLED_RESET 45 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, OLED_RESET);

//CV sequencer
#include "Melody.h"
Melody *melody;

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
boolean parameterActivity;

//PINS
#define TRIGGERINPIN 51
#define RESETINPIN 53
#define RXPIN 17
#define TXPIN 18

#define SDAPIN 20
#define SCLPIN 21

//encoders
#define MAXENCODERS 8  //total of encoders
#define MOODENCODER 0  //MOOD encoder id
#define CROSSENCODER 1 //CROSS encoder id

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

//trigger out
#define TRIGOUTPATTERNPIN1 7
#define TRIGOUTPIN1 5
#define TRIGOUTPIN2 3
#define TRIGOUTPIN3 16
#define TRIGOUTPIN4 29
#define TRIGOUTPIN5 23
#define TRIGOUTPIN6 27

#define PARAMACTIVITY 47

//action buttons
#define ACTIONPININSTR1 6
#define ACTIONPININSTR2 4
#define ACTIONPININSTR3 2
#define ACTIONPININSTR4 17
#define ACTIONPININSTR5 31
#define ACTIONPININSTR6 25

//encoder buttons
#define ENCBUTMOOD 0
#define ENCBUTCROSS 1
#define ENCBUTINSTR1 2
#define ENCBUTINSTR2 3
#define ENCBUTINSTR3 4
#define ENCBUTINSTR4 5
#define ENCBUTINSTR5 6
#define ENCBUTINSTR6 7

//interface
EventDebounce info(30);

//time related and bpm
volatile uint32_t u_lastTick;                   //last time tick was called
volatile uint32_t u_tickInterval = 1000000;     //tick interval
volatile uint32_t safeZoneEndTime;              //safe time zone
boolean updateDisplayUpdateLatencyComp = false; //flag to update latency compensation
int32_t u_LatencyComp = 1100;                   //default latency compensation keep the 100 microsseconds there
int32_t u_LatencyCompStep = 1000;               //latency compensation amount update step
#define u_LatencyCompLimit 20000                //latency compensation + and - limit
uint16_t bpm = 125;
uint32_t u_bpm = 0;

uint8_t powArray[6] = {1, 2, 4, 8, 16, 32};

//encoders buttons : mood, cross, instruments
uint8_t encoderButtonPins[MAXENCODERS] = {ENCBUTPINMOOD, ENCBUTPINCROSS, ENCBUTPININSTR1, ENCBUTPININSTR2, ENCBUTPININSTR3, ENCBUTPININSTR4, ENCBUTPININSTR5, ENCBUTPININSTR6};
boolean encoderButtonState[MAXENCODERS] = {0, 0, 0, 0, 0, 0, 0, 0};

//action pins
uint8_t instrActionPins[G_MAXINSTRUMENTS] = {ACTIONPININSTR1, ACTIONPININSTR2, ACTIONPININSTR3, ACTIONPININSTR4, ACTIONPININSTR5, ACTIONPININSTR6}; //pins
boolean instrActionState[G_MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};

//triggers
volatile int8_t gateLenghtCounter = 0;
uint8_t triggerPins[G_MAXINSTRUMENTS] = {TRIGOUTPIN1, TRIGOUTPIN2, TRIGOUTPIN3, TRIGOUTPIN4, TRIGOUTPIN5, TRIGOUTPIN6};

EventDebounce interfaceEvent(200); //min time in ms to accept another interface event
EventDebounce melodieUpdate(70);   //min time in ms read new melody parameter

Rotary moodEncoderPot(ENCPINAMOOD, ENCPINBMOOD);
Rotary crossEncoderPot(ENCPINACROSS, ENCPINBCROSS);
Rotary instr1encoder(ENCPINAINSTR1, ENCPINBINSTR1);
Rotary instr2encoder(ENCPINAINSTR2, ENCPINBINSTR2);
Rotary instr3encoder(ENCPINAINSTR3, ENCPINBINSTR3);
Rotary instr4encoder(ENCPINAINSTR4, ENCPINBINSTR4);
Rotary instr5encoder(ENCPINAINSTR5, ENCPINBINSTR5);
Rotary instr6encoder(ENCPINAINSTR6, ENCPINBINSTR6);
Rotary *encoders[MAXENCODERS] = {&moodEncoderPot, &crossEncoderPot, &instr1encoder, &instr2encoder, &instr3encoder, &instr4encoder, &instr5encoder, &instr6encoder};

int16_t selectedMood = 0;                     //actual selected mood
int16_t lastSelectedMood = 255;               //prevents to execute 2 times the same action
int16_t previousMoodname = 0;                 //previous mood name
int8_t lastCrossBarGraphValue = 127;          //0 to G_MAXINSTRUMENTS possible values
boolean updateDisplayBrowseMood = false;      //schedule some display update
boolean updateDisplaySelectMood = false;      //schedule some display update
boolean updateDisplayUpdateBpm = false;       //update bpm
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
uint16_t importedMoodsFromSd = 0;
char *moodKitName[G_MAXMEMORYMOODS] = {
    "Empty",
    "P.Labs-Empty Room",
    "P.Labs-Choke",
    "P.Labs-April23",
    "Carlos Pires-Drama"};
//{pattern id, pattern id, pattern id, pattern id, pattern id, pattern id, absolute volume reduction}
uint32_t moodKitData[G_MAXMEMORYMOODS][G_MAXINSTRUMENTS] = {
    {1, 1, 1, 1, 1, 1}, //reserved MUTE = 1
    {2, 2, 2, 2, 2, 2}, //P.Labs-Empty Room
    {2, 3, 3, 4, 3, 1}, //P.Labs-Choke
    {2, 3, 4, 2, 3, 1}, //P.Labs-April23
    {2, 5, 5, 2, 5, 4}  //Carlos Pires-Drama
};

//decks
int8_t crossfader = 0;
int8_t lastCrossfadedValue = G_MAXINSTRUMENTS;

#include "displayProcs.h"

void setup()
{
#if DO_SERIAL == true
  Serial.begin(9600);
  Serial.println("Debugging..");
#endif

  analogWriteResolution(12);
  analogReadResolution(10);
  pinMode(DAC0, OUTPUT);

  //start decks and melody
  mood[0] = new Mood(G_INTERNALMOODS);
  mood[1] = new Mood(G_INTERNALMOODS);
  melody = new Melody(G_MAXSTEPS);
  ladderMenu.setMenu(laddderMenuSeparators, MAXPARAMETERS + 1);

  //import moods from SD card
#if DO_SD == true
  sdc = new SdComm(SD_CS, true);
  sdc->importMoods(moodKitName, moodKitData, G_INTERNALMOODS);
  importedMoodsFromSd = sdc->getimportedRecords();
  //update decks to recognize new moods
  mood[0]->changeMaxMoods(G_INTERNALMOODS + importedMoodsFromSd);
  mood[1]->changeMaxMoods(G_INTERNALMOODS + importedMoodsFromSd);
#endif

  // pinMode(TRIGGERINPIN, INPUT);
  // attachInterrupt(digitalPinToInterrupt(TRIGGERINPIN), ISRswitchToExternal, RISING);

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
  pinMode(TRIGOUTPATTERNPIN1, OUTPUT);
  digitalWrite(TRIGOUTPATTERNPIN1, HIGH);
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    digitalWrite(triggerPins[i], HIGH);
    analogWrite(DAC0, i * 512);
    delay(250);
  }
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    digitalWrite(triggerPins[i], LOW);
    analogWrite(DAC0, (G_MAXINSTRUMENTS - i) * 512);
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

  //setup display Address 0x3D for 128x64
  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS))
  {
#if DO_SERIAL == true
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

//shifted clock and everything to step sequencer related
void ISRfireTimer4()
{
  noInterrupts();
  Timer5.start(G_DEFAULTGATELENGHT);                         //start 5ms first trigger timer
  Timer4.stop();                                             //stop timer shift
  for (uint8_t instr = 0; instr < G_MAXINSTRUMENTS; instr++) //for each instrument
  {
    boolean targetDeck;
    targetDeck = crossfadedDeck(instr);
    //step all ready to play and it wasnt manually muted
    if (mood[targetDeck]->patterns[instr]->playThisStep(stepCount) && !instrActionState[instr])
    {
      digitalWrite(triggerPins[instr], HIGH);
#if DO_WT == true
      wTrig.trackGain(mood[targetDeck]->samples[instr]->getValue(), -6);
      wTrig.trackLoad(mood[targetDeck]->samples[instr]->getValue());
#endif
    }
    mood[targetDeck]->patterns[instr]->tappedStep = false; //reset any tapped step
  }
#if DO_WT == true
  wTrig.resumeAllInSync();
#endif
  interrupts();
  analogWrite(DAC0, melody->getNote());
  gateLenghtCounter = 0;
  stepCount++;
  if (stepCount >= G_MAXSTEPS)
  {
    stepCount = 0;
    melody->resetStepCounter();
  }
  safeZoneEndTime = (micros() + u_tickInterval - DISPLAYUPDATETIME);
}

//close all trigger
//  _ ______ _______ _______ _______
//_| |      |       |       |       |______
void ISRendTriggers()
{
  //compare all triggers times
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    if (mood[crossfadedDeck(i)]->patterns[i]->gateLenghtSize <= gateLenghtCounter)
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
    Timer5.start(G_EXTENDEDGATELENGHT); //start 80ms triggers timers
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
        updateDisplayBrowseMood = true;
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
      else if (laddderMenuOption == COMMANDBPM)
      {
        updateDisplayUpdateBpm = true;
        bpm += encoderChange;
        u_bpm = bpm2micros4ppqn(bpm);
      }
      else
      { //or else , cross change
        crossfader += encoderChange;
        crossfader = constrain(crossfader, 0, G_MAXINSTRUMENTS);
        updateDisplayUpdateCross = encoderChange;
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
        updateDisplayChangePlayingSample = _instrum;
      }
      //gate lenght change
      else if (laddderMenuOption == COMMANDTRL)
      {
        mood[thisDeck]->patterns[_instrum]->changeGateLenghSize(encoderChange);
        updateDisplayUpdategateLenght = _instrum;
      }
      //pattern change
      else if (noOneEncoderButtonIsPressed())
      {
        if (encoderChange == -1)
          mood[thisDeck]->patterns[_instrum]->id->reward();
        else
          mood[thisDeck]->patterns[_instrum]->id->advance();
        updateDisplayInstrPattern = _instrum; //flags to update this instrument on display
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

  //ASYNC updates ==============================================================
  //flags if any display update will be necessary in this cycle
  boolean updateDisplay = false;
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
  else if ((updateDisplayUpdateCross != 0) && safeZone())
  {
    checkDefaultDisplay();
    displayShowCrossBar(crossfader);
    updateDisplayUpdateCross = 0;
    updateDisplay = true;
  }
  //copy selected mood to new deck inside sample update window
  else if (updateDisplaySelectMood && safeZone())
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
          mood[thisDeck]->samples[instr]->setValue(mood[!thisDeck]->samples[instr]->getValue());
          mood[thisDeck]->patterns[instr]->id->setValue(mood[!thisDeck]->patterns[instr]->id->getValue());
          mood[thisDeck]->patterns[instr]->permanentMute = mood[!thisDeck]->patterns[instr]->permanentMute;
          mood[thisDeck]->patterns[instr]->gateLenghtSize = mood[!thisDeck]->patterns[instr]->gateLenghtSize;
        }
        else //or discard
        {
          mood[thisDeck]->discardNotXfadedInstrument(instr);
        }
      }
      lastCrossfadedValue = crossfader;
    }
    //change decks
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
    updateDisplaySelectMood = false;
    updateDisplay = true;
  }
  //if only one intrument pattern changed
  else if (updateDisplayInstrPattern != -1)
  {
    displayShowInstrPattern(updateDisplayInstrPattern, RAM);
    displayUpdateLineArea(3, "Custom");
    displayShowCornerInfo(0, mood[thisDeck]->patterns[updateDisplayInstrPattern]->id->getValue());
    updateDisplay = true;
    updateDisplayInstrPattern = -1;
  }
  //if sampler changed and there is available time to update screen
  else if ((updateDisplayChangePlayingSample != -1) && safeZone())
  {
    displayUpdateLineArea(3, moodKitName[mood[thisDeck]->samples[updateDisplayChangePlayingSample]->getValue() / 6]);
    displayShowCornerInfo(1, mood[thisDeck]->samples[updateDisplayChangePlayingSample]->getValue());
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
    displayEraseInstrumentBlock(updateDisplayEraseInstrumentPattern); //clear pattern from display
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
  //rolback tap
  else if (updateDisplayRollbackInstrumentTap != -1)
  {
    displayShowInstrPattern(updateDisplayRollbackInstrumentTap, RAM); //update display with removed step
    updateDisplay = true;
    updateDisplayRollbackInstrumentTap = -1;
  }
  //if any display update
  if (updateDisplay)
    display.display();

  //SYNCd updates ==============================================================
  //read all encoders buttons (invert boolean state to make easy future comparation)
  for (int8_t i = 0; i < MAXENCODERS; i++)
    encoderButtonState[i] = !digitalRead(encoderButtonPins[i]);
  //if new mood was selected....copy reference tables or create new mood in the cross area
  if (onlyOneEncoderButtonIsPressed(ENCBUTMOOD) && interfaceEvent.debounced())
  {
    interfaceEvent.debounce(1000); //block any other interface event
    updateDisplaySelectMood = true;
  }
  //all action (mute) buttons
  for (int8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    //read action instrument button (invert boolean state to make easy comparation)
    instrActionState[i] = !digitalRead(instrActionPins[i]);
    //if interface available and action button pressed
    if (interfaceEvent.debounced() && instrActionState[i])
    {
      //compare with ladder menu
      switch (laddderMenuOption)
      {
      case COMMANDTAP:
        //compute if the tap will be saved on this step or into next
        int8_t updateDisplayTapStep;
        if (micros() < (u_lastTick + (u_tickInterval >> 1))) //if we are still in the same step
        {
          //DRAGGED STEP - tapped after trigger time
          //updateDisplayTapStep = stepCount-1;
          updateDisplayTapStep = ((stepCount - 1) < 0) ? (G_MAXSTEPS - 1) : (stepCount - 1);
//if this step is empty, play sound
#if DO_WT == true
          //          if (!mood[thisDeck]->patterns[i]->isThisStepActive(mood[thisDeck]->patterns[i]->id->getValue(), updateDisplayTapStep))
          if (!mood[thisDeck]->patterns[i]->getStep(updateDisplayTapStep))
            wTrig.trackPlayPoly(mood[thisDeck]->samples[i]->getValue() + 1); //send a wtrig async play
#endif
        }
        else
        {
          //RUSHED STEP - tapped before trigger time
          //updateDisplayTapStep = ((stepCount) >= G_MAXSTEPS) ? 0 : (stepCount);
          updateDisplayTapStep = stepCount;
          //tap saved to next step
#if DO_WT == true
          wTrig.trackPlayPoly(mood[thisDeck]->samples[i]->getValue() + 1); //send a wtrig async play
#endif
          mood[thisDeck]->patterns[i]->tappedStep = true;
        }
        mood[thisDeck]->patterns[i]->tapStep(updateDisplayTapStep);
        updateDisplayTapInstrumentPattern = i;
        interfaceEvent.debounce(250);
        break;
      case COMMANDUND:
        //if this pattern is a custom one and there was any rollback available
        if ((mood[thisDeck]->patterns[i]->customPattern) &&
            (mood[thisDeck]->patterns[i]->undoAvailable()))
        {
          mood[thisDeck]->patterns[i]->rollbackUndoStep(); //rollback the last saved undo
          displayShowInstrPattern(i, RAM);                 //update display with new inserted step
          updateDisplay = true;
          updateDisplayRollbackInstrumentTap = i;
          interfaceEvent.debounce(200);
        }
        break;
      case COMMANDERS:
        //erase pattern IF possible
        mood[thisDeck]->patterns[i]->eraseInstrumentPattern();
        interfaceEvent.debounce(1000);
        updateDisplayEraseInstrumentPattern = i;
        break;
      case COMMANDSIL:
        mood[thisDeck]->patterns[i]->permanentMute = !mood[thisDeck]->patterns[i]->permanentMute;
        interfaceEvent.debounce(1000);
        break;
      }
    }
  }
  //long interval between readings from action ladder menu
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
    if (melody->readNewMelodyParameter())
    {
      digitalWrite(TRIGOUTPATTERNPIN1, HIGH);
      info.debounce();
      parameterActivity = true;
    }
  }
  if (parameterActivity && info.debounced())
  {
    parameterActivity = false;
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
