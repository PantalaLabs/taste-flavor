/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create melodies
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "tf_Defines.h" //global taste and flavor defines
#include <SPI.h>
#include <SD.h>
#include "Pinout.h" //pinout defines
#include <Arduino.h>
#include <Rotary.h>
#include <PantalaDefines.h>
#include <DueTimer.h>
#include <AnalogInput.h>
#include <Trigger.h>
#include "Mood.h"

File myFile;

//decks
Mood *mood[2];
bool thisDeck = 0;

#ifdef DO_WT
#include <wavTrigger.h>
wavTrigger wTrig;
#endif

#define DISPLAYUPDATETIME 35000 //microsseconds to update display

#include <Adafruit_SSD1306.h>
#define OLED_RESET 43 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, OLED_RESET);
AnalogInput ladderMenu(G_LADDERMENUPIN); ///analog pin
uint8_t laddderMenuOption;               //actual selected ladder menu
int laddderMenuSeparators[MAXPARAMETERS + 1] = {30, 140, 290, 490, 620, 750, 840, 900, 980, G_MAX10BIT};
uint32_t laddderMenuReadDebounce;            //interval to read ladder menu
volatile uint32_t u_lastTick;                //last time tick was called
volatile uint32_t u_tickInterval = 1000000;  //tick interval
volatile uint32_t safeZoneEndTime;           //safe time zone
volatile int8_t stepCount = 0;               //main stepcount
int8_t queuedRotaryEncoder = 0;              //queued rotary encoder
bool updateDisplayUpdateLatencyComp = false; //flag to update latency compensation
int32_t u_LatencyComp = 1100;                //default latency compensation keep the 100 microsseconds there
const int32_t u_LatencyCompStep = 1000;      //latency compensation amount update step
#define u_LatencyCompLimit 20000             //latency compensation + and - limit
uint16_t bpm = 125;                          //actual bpm in bpm
uint32_t u_bpm = 0;                          //actual bpm value in us
uint8_t encoderButtonPins[MAXENCODERS] = {ENCBUTPINMOOD, ENCBUTPINCROSS, ENCBUTPININSTR1, ENCBUTPININSTR2, ENCBUTPININSTR3, ENCBUTPININSTR4, ENCBUTPININSTR5, ENCBUTPININSTR6};
bool encoderButtonState[MAXENCODERS] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t instrActionPins[G_MAXINSTRUMENTS] = {ACTIONPININSTR1, ACTIONPININSTR2, ACTIONPININSTR3, ACTIONPININSTR4, ACTIONPININSTR5, ACTIONPININSTR6}; //pins
bool instrActionState[G_MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
uint32_t instrActionDebounce[G_MAXINSTRUMENTS]; //instrument actions button debounce
volatile int8_t gateLenghtCounter = 0;
uint8_t triggerPins[G_MAXINSTRUMENTS] = {TRIGOUTPIN1, TRIGOUTPIN2, TRIGOUTPIN3, TRIGOUTPIN4, TRIGOUTPIN5, TRIGOUTPIN6};

Trigger trigOutPattern(TRIGOUTPATTERNPIN);

uint32_t interfaceEventDebounce; //min time in ms to accept another interface event

bool externalCLockSourceAllowed = false;

Rotary moodEncoderPot(ENCPINAMOOD, ENCPINBMOOD);
Rotary crossEncoderPot(ENCPINACROSS, ENCPINBCROSS);
Rotary instr1encoder(ENCPINAINSTR1, ENCPINBINSTR1);
Rotary instr2encoder(ENCPINAINSTR2, ENCPINBINSTR2);
Rotary instr3encoder(ENCPINAINSTR3, ENCPINBINSTR3);
Rotary instr4encoder(ENCPINAINSTR4, ENCPINBINSTR4);
Rotary instr5encoder(ENCPINAINSTR5, ENCPINBINSTR5);
Rotary instr6encoder(ENCPINAINSTR6, ENCPINBINSTR6);
Rotary *encoders[MAXENCODERS] = {&moodEncoderPot, &crossEncoderPot, &instr1encoder, &instr2encoder, &instr3encoder, &instr4encoder, &instr5encoder, &instr6encoder};

bool flagUD_newClockSource = false;      //show clock source
bool flagUD_save = false;                //show saved message
bool flagUD_error = false;               //show error message
bool flagUD_browseThisMood = false;      //show browsed mood
bool flagUD_newMoodSelected = false;     //show selected mood
bool flagUD_newBpmValue = false;         //show bpm
int8_t flagUD_newCrossfaderPosition = 0; //show new crossfader position
int8_t flagUD_newPlayingPattern = -1;    //show instrument pattern
int8_t flagUD_newPlayingSample = -1;     //show sample
int8_t flagUD_newGateLenght = -1;        //show gate lenght
int8_t flagUD_eraseThisPattern = -1;     //show selected pattern
int8_t flagUD_tapNewStep = -1;           //show tap new step
int8_t flagUD_rollbackTappedStep = -1;   //show undo tapped step
//bool flag_fromSavedMood = false; // ??????? prepare current deck to leave it the same way user customized
int8_t flag_newCrossfaderValue = 0;     //update new value on crossfader ,  inside safe zone
bool allChannelsMuteState = false;      //stores mute / unmute state
bool internalClockSource = true;        //0=internal , 1=external
bool defaultDisplayNotActiveYet = true; //display not used yet
bool muteAllChannels = false;           //show mute / unmute all channels
int16_t selectedMood = 0;               //actual selected mood
int16_t lastSelectedMood = 255;         //prevents to execute 2 times the same action
int16_t previousMoodname = 0;           //previous mood name
int8_t lastCrossBarGraphValue = 127;    //0 to G_MAXINSTRUMENTS possible values

typedef struct
{
  char name[25];
  uint16_t pattern[6];
  uint16_t sample[6];
} record_type;
record_type moodarray[300];

//mood
uint16_t importedMoodsFromSd = 0;
int8_t crossfader = 0;
int8_t lastCrossfadedValue = G_MAXINSTRUMENTS;

#include "displayProcs.h"

void setup()
{

//serial monitor
#ifdef DEB
  Serial.begin(9600);
#endif
  DEBL("debugging..");

  analogReadResolution(10);

  pinMode(TRIGGERINPIN, INPUT_PULLUP);
  pinMode(RESETINPIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RESETINPIN), ISRresetSeq, FALLING);

  ladderMenu.setMenu(laddderMenuSeparators, MAXPARAMETERS + 1);

  //setup display Address 0x3D for 128x64
  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS))
  {
    DEBL(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed
  }
  else
  {
    DEBL(F("Display OK"));
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

    DEBL(F("ladder menu"));
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE); // white text
    display.cp437(true);
    display.setCursor(0, 0);
    while (!digitalRead(ENCBUTPINCROSS))
    {
      ladderRead = analogRead(G_LADDERMENUPIN);
      DEBL(ladderRead);
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
      DEB(String(analogRead(i + 1)) + ",");
    DEBL(".");
  }

  //if instrument 2 button pressed , show all trigger leds
  if (!digitalRead(ENCBUTPININSTR2))
  {
    trigOutPattern.start();
    for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
      digitalWrite(triggerPins[i], HIGH);
    delay(5000);
    while (digitalRead(ENCBUTPININSTR1))
    {
      //if instrument 2 button pressed again  , exit test
    }
    trigOutPattern.end();
    for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
      digitalWrite(triggerPins[i], LOW);
  }

  //import moods from SD card

  DEB("initializing SD card...");
  if (!SD.begin(SD_CS))
    DEBL("begin failed");
  else
    DEBL("begin done");
  importedMoodsFromSd = SD_loadMoods();
  //SD_importPatterns();
  //update decks to recognize new moods
  // mood[0]->changeMaxMoods(importedMoodsFromSd);
  // mood[1]->changeMaxMoods(importedMoodsFromSd);

  //start decks
  mood[0] = new Mood(importedMoodsFromSd);
  mood[1] = new Mood(importedMoodsFromSd);

#ifdef DO_WT
  // WAV Trigger startup at 57600
  delay(100);
  wTrig.start();
  delay(50);
  wTrig.stopAllTracks();
  wTrig.masterGain(0);
  wTrig.samplerateOffset(0);
  wTrig.setReporting(true);
  delay(100);
#endif

  //if instrument 3 button pressed , display SDA/SCL test
  if (!digitalRead(ENCBUTPININSTR3))
  {
    DEBL(F("display Test"));
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
    DEBL(F("DAC Test"));
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

#ifdef DO_WT
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

  //clock
  u_bpm = bpm2micros4ppqn(bpm);
  attachInterrupt(digitalPinToInterrupt(TRIGGERINPIN), ISRidentifyExtClockSource, FALLING);
  Timer3.attachInterrupt(ISRbaseTimer);
  Timer3.start(u_bpm);
  Timer4.attachInterrupt(ISRfireTimer4);
  Timer5.attachInterrupt(ISRendTriggers);

  DEBL("setup ends.");
  delay(50);
  //END SETUP-----------------------------------------------------------------------------
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

//shifted clock and step sequencer related
void ISRfireTimer4()
{
  noInterrupts();
  Timer5.start(G_DEFAULTGATELENGHT); //start 5ms first trigger timer
  Timer4.stop();                     //stop timer shift

  for (int8_t instr = 0; instr < G_MAXINSTRUMENTS; instr++) //for each instrument
  {
    bool targetDeck;
    targetDeck = crossfadedDeck(instr);

#ifdef DO_WT
    //play a sample if :
    //if pattern ready + it wasnt manually muted
    //it was manually actioned together with laddermenu
    //if (mood[targetDeck]->patterns[instr]->playThisStep(stepCount) && (!instrActionState[instr] || (instrActionState[instr] && (laddderMenuOption < MAXPARAMETERS))))
    if (mood[targetDeck]->patterns[instr]->playThisStep(stepCount) && !instrActionState[instr])
    {
      wTrig.trackGain(mood[targetDeck]->samples[instr]->getValue(), -6);
      wTrig.trackLoad(mood[targetDeck]->samples[instr]->getValue());
    }
#endif
    //fire instrument trigger even if sample was momently silenced
    if (mood[targetDeck]->patterns[instr]->playThisTrigger(stepCount) && !instrActionState[instr])
      digitalWrite(triggerPins[instr], HIGH);

    mood[targetDeck]->patterns[instr]->tappedStep = false; //reset any tapped step
  }
#ifdef DO_WT
  wTrig.resumeAllInSync();
#endif
  interrupts();
  gateLenghtCounter = 0;
  if ((stepCount == 0) || (stepCount == 60) || (stepCount == 56))
    trigOutPattern.start();
  NEXT(stepCount, G_MAXSTEPS);
  safeZoneEndTime = (micros() + u_tickInterval - DISPLAYUPDATETIME);
}

void ISRidentifyExtClockSource()
{
  if (externalCLockSourceAllowed)
  {
    flagUD_newClockSource = true;
    internalClockSource = false;
  }
}

void ISRresetSeq()
{
  if (u_LatencyComp >= 0)
    stepCount = 0;
  else
    stepCount = 1;
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
    //Timer5.stop();                      //stops trigger timer
    Timer5.start(G_EXTENDEDGATELENGHT); //start another G_EXTENDEDGATELENGHT _US trigger timer
  }
}

//allows to change samples only upo to after 2/3 of the tick interval
//to avoid to change sample the same time it was triggered
bool safeZone()
{
  return (micros() < safeZoneEndTime);
}

//return if this instrument belongs to this or not to this deck
bool crossfadedDeck(uint8_t _instr)
{
  return (_instr < crossfader) ? thisDeck : !thisDeck;
}

//read queued encoder status
void processRotaryEncoder()
{
  NEXT(queuedRotaryEncoder, MAXENCODERS);
  unsigned char result = encoders[queuedRotaryEncoder]->process();
  if ((result == 16) || (result == 32))
  {
    int8_t encoderChange;
    encoderChange = (result == 16) ? 1 : -1;
    if (queuedRotaryEncoder == MOODENCODER)
    {
      selectedMood += encoderChange;
      selectedMood = constrain(selectedMood, 0, importedMoodsFromSd - 1);
      if (lastSelectedMood != selectedMood)
      {
        flagUD_browseThisMood = true;
        lastSelectedMood = selectedMood;
      }
    }
    else if (queuedRotaryEncoder == CROSSENCODER)
    {
      //if latency menu pressed together , change timer shift
      if (laddderMenuOption == COMMANDLAT)
      {
        u_LatencyComp += encoderChange * u_LatencyCompStep;
        u_LatencyComp = constrain(u_LatencyComp, -u_LatencyCompLimit, u_LatencyCompLimit);
        updateDisplayUpdateLatencyComp = true;
      }
      //if cross button is pressed and cross rotate CHANGE BPM
      if (onlyOneEncoderButtonIsPressed(ENCBUTCROSS) && (millis() > interfaceEventDebounce))
      {
        flagUD_newBpmValue = true;
        bpm += encoderChange;
        bpm = constrain(bpm, G_MINBPM, G_MAXBPM);
        u_bpm = bpm2micros4ppqn(bpm);
        interfaceEventDebounce = millis() + 50; //block any other interface event
      }
      else
      { //or else , cross change
        //flags to update crossfader value ONLY inside the safe zone
        flag_newCrossfaderValue = crossfader + encoderChange;
        flag_newCrossfaderValue = constrain(flag_newCrossfaderValue, 0, G_MAXINSTRUMENTS);
        // crossfader += encoderChange;
        // crossfader = constrain(crossfader, 0, G_MAXINSTRUMENTS);
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
        mood[thisDeck]->patterns[_instrum]->changeGateLenghSize(encoderChange);
        flagUD_newGateLenght = _instrum;
      }
      //pattern change
      else if (noneEncoderButtonIsPressed())
      {
        if (encoderChange == -1)
          mood[thisDeck]->patterns[_instrum]->patternIndex->reward();
        else
          mood[thisDeck]->patterns[_instrum]->patternIndex->advance();
        flagUD_newPlayingPattern = _instrum; //flags to update this instrument on display
      }
    }
  }
}

//verify if NO ONE encoder button is pressed
bool noneEncoderButtonIsPressed()
{
  for (uint8_t i = 0; i < MAXENCODERS; i++) //search all encoder buttons
    if (encoderButtonState[i])              //it is pressed
      return false;
  //if all tests where OK
  return true;
}

//verify if ONLY ONE encoder button is pressed
bool onlyOneEncoderButtonIsPressed(uint8_t _target)
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
bool twoEncoderButtonsArePressed(uint8_t _target1, uint8_t _target2)
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
  if ((!externalCLockSourceAllowed) && (millis() > 5000))
    externalCLockSourceAllowed = true;

  //read queued encoders
  processRotaryEncoder();

  //ASYNC'd updates ==============================================================
  //flags if any display update will be necessary in this cycle
  bool updateDisplay = false;
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
    displayShowCornerInfo("BPM", bpm);
    updateDisplay = true;
  }
  //cross bar changes inside sample update window
  //this could be took off if using Tsunami
  else if ((flagUD_newCrossfaderPosition != 0) && safeZone())
  {
    crossfader = flag_newCrossfaderValue;
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
    // if (!flag_fromSavedMood)
    // {
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
              mood[!thisDeck]->patterns[instr]->patternIndex->getValue(),
              mood[!thisDeck]->patterns[instr]->muted,
              mood[!thisDeck]->patterns[instr]->gateLenghtSize);
        }
        else //or discard
        {
          mood[thisDeck]->discardNotXfadedInstrument(instr);
        }
      }
      lastCrossfadedValue = crossfader;
      //  }
    }
    //switch decks
    crossfader = 0;
    thisDeck = !thisDeck;
    mood[thisDeck]->cue(selectedMood,
                        moodarray[selectedMood].pattern[0], //
                        moodarray[selectedMood].pattern[1], //
                        moodarray[selectedMood].pattern[2], //
                        moodarray[selectedMood].pattern[3], //
                        moodarray[selectedMood].pattern[4], //
                        moodarray[selectedMood].pattern[5], //
                        moodarray[selectedMood].sample[0],  //
                        moodarray[selectedMood].sample[1],  //
                        moodarray[selectedMood].sample[2],  //
                        moodarray[selectedMood].sample[3],  //
                        moodarray[selectedMood].sample[4],  //
                        moodarray[selectedMood].sample[5]   //
    );
    displayUpdateLineArea(1, moodarray[previousMoodname].name);
    displayUpdateLineArea(2, moodarray[selectedMood].name);
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
    displayShowCornerInfo("PAT", mood[thisDeck]->patterns[flagUD_newPlayingPattern]->patternIndex->getValue());
    updateDisplay = true;
    flagUD_newPlayingPattern = -1;
  }
  //if sampler changed and there is available time to update screen
  else if ((flagUD_newPlayingSample != -1) && safeZone())
  {
    displayUpdateLineArea(3, moodarray[mood[thisDeck]->samples[flagUD_newPlayingSample]->getValue() / 6].name);
    displayShowCornerInfo("SMP", mood[thisDeck]->samples[flagUD_newPlayingSample]->getValue());
    updateDisplay = true;
    flagUD_newPlayingSample = -1;
  }
  //if latency compensation changed
  else if (updateDisplayUpdateLatencyComp)
  {
    displayShowCornerInfo("ms", (u_LatencyComp - 100) / 1000);
    updateDisplay = true;
    updateDisplayUpdateLatencyComp = false;
  }
  //if gate lenght changed
  else if (flagUD_newGateLenght != -1)
  {
    displayShowCornerInfo("LEN", (G_DEFAULTGATELENGHT + (mood[crossfadedDeck(flagUD_newGateLenght)]->patterns[flagUD_newGateLenght]->gateLenghtSize * G_EXTENDEDGATELENGHT)) / 1000);
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
  //rollback tap
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
      attachInterrupt(digitalPinToInterrupt(TRIGGERINPIN), ISRidentifyExtClockSource, FALLING);
      Timer3.attachInterrupt(ISRbaseTimer);
      Timer3.start(u_bpm);
    }
    else
    {
      Timer3.stop();
      Timer3.detachInterrupt();
      attachInterrupt(digitalPinToInterrupt(TRIGGERINPIN), ISRbaseTimer, FALLING);
    }
    displayShowCornerInfo((internalClockSource) ? "INT" : "EXT");
    updateDisplay = true;
  }
  else if (flagUD_save)
  {
    flagUD_save = false;
    bool targetDeck;
    String moodName, moodString;
    String availChars = "abcdefghijklmnopqrstuvxz";

    //when saving a mood we have to save patterns first and point to then
    //save customized and unempty pattern to file , get the its new index ,
    //and then reference it inside the new mood
    //or reference to 1 in case they are empty
    //DO NOT JUMP to new mood.
    //only saves and keep working with 2 actual loaded moods
    DEBL("saving new mood");

    //mood index
    moodString = String(importedMoodsFromSd);
    moodString += ",";

    //set default patterns and samples
    uint8_t virtualPatterns[G_MAXINSTRUMENTS] = {1, 1, 1, 1, 1, 1};
    uint8_t virtualSamples[G_MAXINSTRUMENTS] = {0, 1, 2, 3, 4, 5};
    //bool virtualSteps[G_MAXSTEPS];

    //verify any custom pattern , save it before save mood
    for (int8_t instr = 0; instr < G_MAXINSTRUMENTS; instr++)
    {
      bool targetDeck;
      targetDeck = crossfadedDeck(instr);
      //if this is a custom pattern
      if (mood[targetDeck]->patterns[instr]->customPattern != 0)
      {
        //it is not muted and not empty
        if (!mood[targetDeck]->patterns[instr]->muted &&
            (!mood[targetDeck]->patterns[instr]->emptyPattern()))
        {
          //save new unempty pattern to file before reference it on new mood
          // for (int8_t i = 0; i < G_MAXINSTRUMENTS; i++)
          //   virtualSteps[i] = mood[targetDeck]->patterns[instr]->getStep(i);
          uint8_t newPatternIndex = SD_dumpNewPattern(instr);
          if (newPatternIndex != 0)
          {
            virtualPatterns[instr] = newPatternIndex;
            virtualSamples[instr] = mood[targetDeck]->samples[instr]->getValue();
            mood[targetDeck]->patterns[instr]->patternIndex->increaseEnd(1);
            mood[!targetDeck]->patterns[instr]->patternIndex->increaseEnd(1);
            flagUD_error = false;
          }
          else
          {
            //error to save pattern into file
            flagUD_error = true;
          }
        }
      }
      else
      {
        //get actual data
        virtualPatterns[instr] = mood[targetDeck]->patterns[instr]->patternIndex->getValue();
        virtualSamples[instr] = mood[targetDeck]->samples[instr]->getValue();
      }
    }

    if (!flagUD_error)
    { //random mood name
      randomSeed(micros());
      moodName = "";
      for (uint8_t i = 0; i < 7; i++)
      {
        uint8_t randomchar = random(0, 1000) % 25;
        moodName += availChars.substring(randomchar, randomchar + 1);
      }
      moodString += String(moodName);
      moodString += ",";
      //update RAM mood array
      moodName.toCharArray(moodarray[importedMoodsFromSd].name, moodName.length());
      //for each pattern
      for (int8_t instr = 0; instr < G_MAXINSTRUMENTS; instr++)
      {
        bool targetDeck;
        targetDeck = crossfadedDeck(instr);
        //update RAM with virtual created mood data
        moodarray[importedMoodsFromSd].pattern[instr] = virtualPatterns[instr];
        moodarray[importedMoodsFromSd].sample[instr] = virtualSamples[instr];
      }
      //dump RAM record to txt
      SD_dumpNewMood(importedMoodsFromSd, moodName,                                                                                                                                                                                                                                    //
                     moodarray[importedMoodsFromSd].pattern[0], moodarray[importedMoodsFromSd].pattern[1], moodarray[importedMoodsFromSd].pattern[2], moodarray[importedMoodsFromSd].pattern[3], moodarray[importedMoodsFromSd].pattern[4], moodarray[importedMoodsFromSd].pattern[5], //
                     moodarray[importedMoodsFromSd].sample[0], moodarray[importedMoodsFromSd].sample[1], moodarray[importedMoodsFromSd].sample[2], moodarray[importedMoodsFromSd].sample[3], moodarray[importedMoodsFromSd].sample[4], moodarray[importedMoodsFromSd].sample[5]);      //
      importedMoodsFromSd++;
      displayShowCornerInfo("SAV");
    }
    updateDisplay = true;
  }
  else if (muteAllChannels)
  {
    muteAllChannels = false;
    allChannelsMuteState = !allChannelsMuteState;
    mood[thisDeck]->setMuteAllInstruments(allChannelsMuteState);
    mood[!thisDeck]->setMuteAllInstruments(allChannelsMuteState);
  }

  if (flagUD_error)
  {
    flagUD_error = false;
    displayShowCornerInfo("ERR");
  }

  //if any display update
  if (updateDisplay)
    display.display();

  //SYNCd updates ==============================================================
  if ((millis() > interfaceEventDebounce))
  {
    //read all encoders buttons (invert bool state to make it easyer for compararison)
    for (int8_t i = 0; i < MAXENCODERS; i++)
    {
      encoderButtonState[i] = !digitalRead(encoderButtonPins[i]);
      //for any any INSTRUMENT encoder button
      if (i >= 2)
      {
        //if it was pressed with menu command silence, silence currently UNloaded deck
        if (laddderMenuOption == COMMANDSIL && encoderButtonState[i])
        {
          mood[!thisDeck]->patterns[i - 2]->muted = !mood[!thisDeck]->patterns[i - 2]->muted;
          interfaceEventDebounce = millis() + 300;
        }
      }
    }
    //mute / unmute all channels
    if ((laddderMenuOption == COMMANDNUL) && encoderButtonState[3])
    {
      muteAllChannels = true;
      interfaceEventDebounce = millis() + 500;
    }
    //save this mood
    if ((laddderMenuOption == COMMANDNUL) && (encoderButtonState[6] && encoderButtonState[7]))
    {
      flagUD_save = true;
      interfaceEventDebounce = millis() + 1000;
    }
  }

  //if new mood was selected....copy reference tables or create new mood in the cross area
  if (onlyOneEncoderButtonIsPressed(ENCBUTMOOD) && (millis() > interfaceEventDebounce))
  {
    interfaceEventDebounce = millis() + 1000; //block any other interface event
    flagUD_newMoodSelected = true;
  }

  //all action (mute) buttons ===============================================================
  for (int8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    //read action instrument button (invert bool state to make easy comparation)
    instrActionState[i] = !digitalRead(instrActionPins[i]);
    //if interface available and action button pressed
    if ((millis() > instrActionDebounce[i]) && instrActionState[i])
    {
      uint16_t makeDebounce = 0;
      //selected ladder menu option
      switch (laddderMenuOption)
      {
      case COMMANDGHO:
#ifdef DO_WT
        wTrig.trackPlayPoly(mood[thisDeck]->samples[i]->getValue()); //send a wtrig async play
#endif
        makeDebounce = u_tickInterval / 1000;
        break;
      case COMMANDTAP:
        //compute if the tap will be saved on this step or into next
        int8_t updateDisplayTapStep;
        if (micros() < (u_lastTick + (u_tickInterval >> 1))) //if we are still in the same step
        {
          //DRAGGED STEP - tapped after trigger time
          //update display tapping previous step
          updateDisplayTapStep = ((stepCount - 1) < 0) ? (G_MAXSTEPS - 1) : (stepCount - 1);
#ifdef DO_WT
          //if this step is empty, play sound
          //if this is a valid step for this deck and this instrument isnt silenced
          if (!mood[thisDeck]->patterns[i]->getStep(updateDisplayTapStep) && !mood[thisDeck]->patterns[i]->muted)
            wTrig.trackPlayPoly(mood[thisDeck]->samples[i]->getValue()); //send a wtrig async play
#endif
        }
        else
        {
          //RUSHED STEP - tapped before trigger time
          //update display tapping current step
          updateDisplayTapStep = stepCount;
#ifdef DO_WT
          //if this instrument isnt silenced
          if (!mood[thisDeck]->patterns[i]->muted)
            wTrig.trackPlayPoly(mood[thisDeck]->samples[i]->getValue()); //send a wtrig async play
#endif
          mood[thisDeck]->patterns[i]->tappedStep = true;
        }
        mood[thisDeck]->patterns[i]->tapStep(updateDisplayTapStep);
        flagUD_tapNewStep = i;
        makeDebounce = (u_tickInterval / 1000) << 1; //this is a special debounce time to avoid 1/4 difference and keep 1/2 distance from one tap to another
        break;
      case COMMANDUND:
        //if this pattern is a custom one and there was any rollback available
        if (mood[thisDeck]->patterns[i]->undoLastTappedStep())
        {
          displayShowInstrPattern(i, RAM); //update display with new inserted step
          updateDisplay = true;
          flagUD_rollbackTappedStep = i;
        }
        makeDebounce = 1000;
        break;
      case COMMANDERS:
        //erase pattern if possible
        mood[thisDeck]->patterns[i]->erasePattern();
        flagUD_eraseThisPattern = i;
        makeDebounce = 1000;
        break;
      case COMMANDSIL:
        //silence only currently loaded deck
        mood[thisDeck]->patterns[i]->muted = !mood[thisDeck]->patterns[i]->muted;
        makeDebounce = 1000;
        break;
      case COMMANDSOL:
        //solo an instrument
        mood[thisDeck]->setSoloInstrument(i);
        makeDebounce = 1000;
        break;
      }
      if (makeDebounce > 0)
        instrActionDebounce[i] = millis() + makeDebounce;
    }
  }
  //long interval between readings from action ladder menu
  if (millis() > laddderMenuReadDebounce)
  {
    laddderMenuOption = ladderMenu.getRawMenuIndex();
    laddderMenuReadDebounce = millis() + 100;
  }

  //2 seconds without external trigger , becomes internal timer
  if (!internalClockSource & (micros() > (u_lastTick + 2000000)))
  {
    flagUD_newClockSource = true;
    internalClockSource = true;
  }

  trigOutPattern.compute();
}

uint16_t SD_loadMoods()
{
  uint16_t importedRecords;
  DEBL("reading moods.txt");
  myFile = SD.open("moods.txt", O_READ);
  if (myFile)
  {
    char readChar;
    uint16_t moodIndex;
    importedRecords = 0;
    while (myFile.available())
    {
      //read one mood at time
      importedRecords++;
      for (uint8_t i = 0; i < 14; i++)
      {
        //clear token
        String token = "";
        readChar = myFile.read(); //skip CRLFs
        if (readChar == '\n')
          readChar = myFile.read();
        while ((readChar != '\n') && (readChar != ',') && (myFile.available()))
        {
          token.concat(readChar);
          readChar = myFile.read();
        }
        if (i == 0)
          moodIndex = token.toInt();
        else if (i == 1)
          token.toCharArray(moodarray[moodIndex].name, token.length() + 1);
        else if ((i >= 2) && (i <= 7))
          moodarray[moodIndex].pattern[i - 2] = token.toInt();
        else if ((i >= 8) && (i <= 13))
          moodarray[moodIndex].sample[i - 8] = token.toInt();
      }
    }
    DEB("imported " + String(importedRecords) + " moods");
  }
  else
    DEBL("failed to open moods.txt");
  DEBL("moods.txt closed.");
  myFile.close();
  return importedRecords;
}

uint8_t SD_dumpNewPattern(uint8_t instr)
{
  String patternString = "";
  String filename = "pattern" + String(instr) + ".txt";
  DEBL("opening and dumping " + filename);
  myFile = SD.open(filename, FILE_WRITE);
  if (myFile)
  {
    bool targetDeck;
    targetDeck = crossfadedDeck(instr);
    uint8_t newIdx = mood[targetDeck]->patterns[instr]->patternIndex->getEnd();
    patternString = String(newIdx) + ",";
    for (uint8_t i = 0; i < G_MAXSTEPS; i++)
    {
      bool myStep = mood[targetDeck]->patterns[instr]->getStep(i);
      patternString += String(myStep);
      mood[targetDeck]->patterns[instr]->setStep(newIdx, i, myStep);
      mood[!targetDeck]->patterns[instr]->setStep(newIdx, i, myStep);
    }
    myFile.println(patternString);
    myFile.close();
    DEBL(patternString);
    DEBL("end dump");

    return newIdx;
  }
  else
  {
    DEBL("open to write failed");
    myFile.close();
    DEBL("end dump");
    return 0;
  }
}

bool SD_dumpNewMood(uint8_t idx, String name, uint8_t _p0, uint8_t _p1, uint8_t _p2, uint8_t _p3, uint8_t _p4, uint8_t _p5, uint8_t _s0, uint8_t _s1, uint8_t _s2, uint8_t _s3, uint8_t _s4, uint8_t _s5)
{
  DEBL("opening moods.txt for dump");
  myFile = SD.open("moods.txt", FILE_WRITE);
  if (myFile)
  {
    String moodString = String(idx) + "," + name + "," + String(_p0) + "," + String(_p1) + "," + String(_p2) + "," + String(_p3) + "," + String(_p4) + "," + String(_p5) + "," + String(_s0) + "," + String(_s1) + "," + String(_s2) + "," + String(_s3) + "," + String(_s4) + "," + String(_s5);
    myFile.println(moodString);
    DEBL(moodString + "\ndump mood end");
  }
  else
    DEBL("open to write failed");
  myFile.close();
  DEBL("end dump");
}

// bool SD_deleteFile(String _fileName)
// {
//   DEB(_fileName);
//   if (!SD.remove(_fileName))
//   {
//     DEBL(" delete failed");
//     return false;
//   }
//   else
//   {
//     DEBL(" deleted");
//     return true;
//   }
// }
