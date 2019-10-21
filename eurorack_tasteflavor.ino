/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create melodies
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.

+---------------+
|refPattern1    |
  +---------------+
  |refPattern2    |      -----+
        ...                   |
    +---------------+         |
    |refPattern6    |         |
    +---------------+         |
                              |
                              V
                   +--------------------+            +-------------------+          +---------------------+
                   |instrXPatternPointer|            |instrXsamplePointer| <--------|rpi available sample |
                   +--------------------+            +-------------------+          +---------------------+
                               |                                |
                               +--------------------------------+
                                               |
                                               V
*/

#define DO_SERIAL false
#define DO_MIDIUSB false
#define DO_MIDIHW true

#ifdef DO_MIDIHW
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();
#endif

#ifdef DO_MIDIUSB
#include "MIDIUSB.h"
#endif

#include <PantalaDefines.h>
#include <Counter.h>
#include <EventDebounce.h>
#include <Rotary.h>
#include <DueTimer.h>

#define I2C_ADDRESS 0x3C
#define DISPLAY_WIDTH 128 // OLED Oled width, in pixels
#define DISPLAY_HEIGHT 64 // OLED Oled height, in pixels
#define TEXTLINE_HEIGHT 9 // OLED Oled height, in pixels

//adafruit
#include <Adafruit_SSD1306.h>
#define OLED_RESET 47 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, OLED_RESET);

#define OLEDUPDATETIME 35000 //microsseconds to update display
#define MAXSTEPS 64          //max step sequence
#define MYCHANNEL 1          //midi channel for data transmission
#define MAXENCODERS 8        //total of encoders
#define MOODENCODER 0        //MOOD encoder id
#define CROSSENCODER 1       //CROSS encoder id
#define MAXINSTRUMENTS 6     //total of instruments
#define MIDICHANNEL 1        //standart drum midi channel
#define MOODMIDINOTE 20      //midi note to mood message
#define CROSSMIDINOTE 21     //midi note to cross message
#define KICKCHANNEL 0
#define SNAKERCHANNEL 1 //snare + shaker
#define CLAPCHANNEL 2
#define HATCHANNEL 3
#define OHHRIDECHANNEL 4 //open hi hat + ride
#define PERCCHANNEL 5
#define MAXINSTR1SAMPLES 3
#define MAXINSTR2SAMPLES 4
#define MAXINSTR3SAMPLES 5
#define MAXINSTR4SAMPLES 5
#define MAXINSTR5SAMPLES 5
#define MAXINSTR6SAMPLES 3
#define MAXINSTR1PATTERNS 2 //remember to discard the [0] position reserved to BKP a pattern
#define MAXINSTR2PATTERNS 3
#define MAXINSTR3PATTERNS 4
#define MAXINSTR4PATTERNS 3
#define MAXINSTR5PATTERNS 4
#define MAXINSTR6PATTERNS 2
#define DOTGRIDINIT 36
#define GRIDPATTERNHEIGHT 4
#define BKPPATTERN 0

//PINS
#define SDAPIN 20
#define SCLPIN 21
#define MIDITXPIN 18
#define ENCBUTPINMOOD 11
#define ENCBUTPINCROSS 8
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
#define ENCPINACROSS 10
#define ENCPINBCROSS 9
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
#define ENCBUTCROSS 1
#define ENCBUTINSTR1 2
#define ENCBUTINSTR2 3
#define ENCBUTINSTR3 4
#define ENCBUTINSTR4 5
#define ENCBUTINSTR5 6
#define ENCBUTINSTR6 7

//time related and bpm
volatile uint32_t u_lastTick;               //last time tick was called
volatile uint32_t u_tickInterval = 1000000; //tick interval
boolean updateOledUpdateTimeShift = false;  //flag to update time shift
int32_t u_timeShift = 1100;                 //default time shift keep the 100 microsseconds there
int32_t u_timeShiftStep = 1000;             //time shift amount update step
#define u_timeShiftLimit 20000              //time shift + and - limit
volatile uint32_t sampleChangeWindowEndTime;
uint16_t bpm = 125;
uint32_t u_bpm = 0;

//MIDI
uint8_t instrSampleMidiNote[MAXINSTRUMENTS] = {10, 11, 12, 13, 14, 15};  //midi note to sample message
uint8_t instrPatternMidiNote[MAXINSTRUMENTS] = {20, 21, 22, 23, 24, 25}; //midi note to pattern message

#define MAXGATELENGHTS 8
#define DEFAULTGATELENGHT 5000
#define EXTENDEDGATELENGHT 40000
boolean thisDeck = 0;
boolean previousDeck = 1;
//triggers
volatile int8_t gateLenghtCounter = 0;
uint8_t triggerPins[MAXINSTRUMENTS] = {TRIGOUTPIN1, TRIGOUTPIN2, TRIGOUTPIN3, TRIGOUTPIN4, TRIGOUTPIN5, TRIGOUTPIN6};
int8_t gateLenghtSize[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};

//encoders buttons : mood, cross, instruments
uint8_t encoderButtonPins[MAXENCODERS] = {ENCBUTPINMOOD, ENCBUTPINCROSS, ENCBUTPININSTR1, ENCBUTPININSTR2, ENCBUTPININSTR3, ENCBUTPININSTR4, ENCBUTPININSTR5, ENCBUTPININSTR6};
boolean encoderButtonState[MAXENCODERS] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t instrActionPins[MAXINSTRUMENTS] = {ACTIONPININSTR1, ACTIONPININSTR2, ACTIONPININSTR3, ACTIONPININSTR4, ACTIONPININSTR5, ACTIONPININSTR6}; //pins
boolean instrActionState[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
boolean permanentMute[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
uint8_t sampleButtonModifier[MAXENCODERS] = {0, 0, ENCBUTINSTR2, ENCBUTINSTR1, ENCBUTINSTR4, ENCBUTINSTR3, ENCBUTINSTR6, ENCBUTINSTR5}; //this points to each instrument sample change button modifier

EventDebounce interfaceEvent(200); //min time in ms to accept another interface event

Rotary MOODencoder(ENCPINAMOOD, ENCPINBMOOD);
Rotary CROSSencoder(ENCPINACROSS, ENCPINBCROSS);
Rotary instr1encoder(ENCPINAINSTR1, ENCPINBINSTR1);
Rotary instr2encoder(ENCPINAINSTR2, ENCPINBINSTR2);
Rotary instr3encoder(ENCPINAINSTR3, ENCPINBINSTR3);
Rotary instr4encoder(ENCPINAINSTR4, ENCPINBINSTR4);
Rotary instr5encoder(ENCPINAINSTR5, ENCPINBINSTR5);
Rotary instr6encoder(ENCPINAINSTR6, ENCPINBINSTR6);
Rotary *encoders[MAXENCODERS] = {&MOODencoder, &CROSSencoder, &instr1encoder, &instr2encoder, &instr3encoder, &instr4encoder, &instr5encoder, &instr6encoder};

boolean updateOledBrowseMood = false;      //schedule some Oled update
boolean updateOledSelectMood = false;      //schedule some Oled update
boolean updateOledUpdateBpm = false;       //update bpm
int16_t selectedMood = 0;                  //actual selected mood
int16_t lastSelectedMood = 255;            //prevents to execute 2 times the same action
int8_t previousMood = 0;                   //previous mood name
int8_t lastCrossBarGraphValue = 127;       //0 to MAXINSTRUMENTS possible values
int8_t updateOledUpdateCross = 0;          //schedule some Oled update
int8_t updateOledInstrPattern = -1;        //update only one instrument pattern
int8_t updateOledChangePlayingSample = -1; //update sample on rasp pi
int8_t updateOledUpdategateLenght = -1;    //update gate lenght on right upper corner
int8_t updateOledEraseInstrumentPattern = -1;
int8_t updateOledTapInstrumentPattern = -1;
int8_t updateOledRollbackInstrumentTap = -1;
boolean updateOledModifierPressed = false;
boolean defaultOledNotActiveYet = true;

#include "patterns.h"
uint8_t customizedPattern[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
boolean isCustomPattern[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};

//sequencer
volatile int8_t stepCount = -1;
int8_t queuedEncoder = 0;

//mood
#define MAXMOODS 4 //max moods
String moodKitName[MAXMOODS] = {"", "P.Labs-Choke", "P.Labs-Empty Room", "P.Labs-April23"};
int8_t moodKitRefPointers[MAXMOODS][(2 * MAXINSTRUMENTS)] = {
    //s, s, s, s, s, s, p, p, p, p, p, p
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1}, //reserved MUTE = 1
    {1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1}, //P.Labs-Choke
    {2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 1}, //P.Labs-Empty Room
    {3, 3, 3, 3, 3, 1, 2, 3, 4, 2, 4, 1}  //P.Labs-April23
};

//decks
#define RAM 0
#define ROM 1
int8_t crossfader = 0;

Counter deck0Sample1(MAXINSTR1SAMPLES + 1);
Counter deck0Sample2(MAXINSTR2SAMPLES + 1);
Counter deck0Sample3(MAXINSTR3SAMPLES + 1);
Counter deck0Sample4(MAXINSTR4SAMPLES + 1);
Counter deck0Sample5(MAXINSTR5SAMPLES + 1);
Counter deck0Sample6(MAXINSTR6SAMPLES + 1);
Counter deck1Sample1(MAXINSTR1SAMPLES + 1);
Counter deck1Sample2(MAXINSTR2SAMPLES + 1);
Counter deck1Sample3(MAXINSTR3SAMPLES + 1);
Counter deck1Sample4(MAXINSTR4SAMPLES + 1);
Counter deck1Sample5(MAXINSTR5SAMPLES + 1);
Counter deck1Sample6(MAXINSTR6SAMPLES + 1);

Counter deck0Pattern1(MAXINSTR1PATTERNS);
Counter deck0Pattern2(MAXINSTR2PATTERNS);
Counter deck0Pattern3(MAXINSTR3PATTERNS);
Counter deck0Pattern4(MAXINSTR4PATTERNS);
Counter deck0Pattern5(MAXINSTR5PATTERNS);
Counter deck0Pattern6(MAXINSTR6PATTERNS);
Counter deck1Pattern1(MAXINSTR1PATTERNS);
Counter deck1Pattern2(MAXINSTR2PATTERNS);
Counter deck1Pattern3(MAXINSTR3PATTERNS);
Counter deck1Pattern4(MAXINSTR4PATTERNS);
Counter deck1Pattern5(MAXINSTR5PATTERNS);
Counter deck1Pattern6(MAXINSTR6PATTERNS);

Counter *deckSamples[2][MAXINSTRUMENTS] = { //
    {&deck0Sample1, &deck0Sample2, &deck0Sample3, &deck0Sample4, &deck0Sample5, &deck0Sample6},
    {&deck1Sample1, &deck1Sample2, &deck1Sample3, &deck1Sample4, &deck1Sample5, &deck1Sample6}};
Counter *deckPatterns[2][MAXINSTRUMENTS] = { //
    {&deck0Pattern1, &deck0Pattern2, &deck0Pattern3, &deck0Pattern4, &deck0Pattern5, &deck0Pattern6},
    {&deck1Pattern1, &deck1Pattern2, &deck1Pattern3, &deck1Pattern4, &deck1Pattern5, &deck1Pattern6}};

boolean playingPatterns[2][6][MAXSTEPS]; //patterns copied from refPatterns to play with

#define MAXUNDOS MAXSTEPS
int8_t undoStack[MAXUNDOS][MAXINSTRUMENTS];

String rightCornerInfo[5] = {"pat", "smp", "bpm", "ms", "len"};

void ISRfireTimer3();
void ISRfireTimer4();
void playMidi(uint8_t _note, uint8_t _velocity, uint8_t _channel);
void readEncoder();
void checkDefaultOled();
void oledWelcome();
void oledEraseBrowsedMoodName();
void oledShowBrowsedMood();
void oledShowCrossBar();
void oledShowCornerInfo(uint8_t _parm, int16_t _val);
void oledEraseInstrumentPattern(uint8_t _instr);
void oledShowInstrumPattern(uint8_t _instr, boolean _source);
void oledShowPreviousMood();
void oledCustomMoodName();
void oledShowSelectedMood();
void endTriggers();
void resetAllPermanentMute();
void resetAllGateLenght();
void restoreCustomizedPatternsToOriginalRef();
boolean sampleUpdateWindow();
boolean noOneEncoderButtonIsPressed();
boolean onlyOneEncoderButtonIsPressed(uint8_t target);
boolean twoEncoderButtonsArePressed(uint8_t _target1, uint8_t _target2);
void eraseInstrumentRam1Pattern(uint8_t _instr);
void setStepIntoplayingPattern(uint8_t _instr, uint8_t _step, uint8_t _val);
void clearUndoArray(uint8_t _instr);
void setStepIntoRefPattern(uint8_t _instr, uint8_t _pattern, uint8_t _step, uint8_t _val);
void setStepIntoUndoArray(uint8_t _instr, uint8_t _step, uint8_t _val);
void setAllPatternAsOriginal();
void setThisPatternAsOriginal(uint8_t _instr);
void setThisPatternAsCustom(uint8_t _instr);
void rollbackStepIntoUndoArray(uint8_t _instr);
void copyRefPatternToThisDeckPlayingPatterns(uint8_t _instr, uint8_t _romPatternTablePointer);
void copyRefPatternToRefPattern(uint8_t _instr, uint8_t _source);
void copyRamPatternToRamPattern(uint8_t _src, uint8_t _trg, uint8_t _instr);

void setup()
{
#ifdef DO_MIDIHW
  MIDI.begin();
#endif

#ifdef DO_SERIAL
  Serial.begin(9600);
  Serial.println("Debugging..");
#endif

  //pinMode(TRIGGERINPIN, INPUT);
  //triggerIn.attachCallOnRising(ISRfireTimer3);

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

  for (uint8_t undos = 0; undos < MAXUNDOS; undos++)
    for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++)
      undoStack[undos][instr] = -1;

  for (uint8_t deck = 0; deck < 2; deck++)
  {
    for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++)
    {
      deckPatterns[deck][instr]->setCyclable(false);
      deckPatterns[deck][instr]->setInit(1);
      deckSamples[deck][instr]->setCyclable(false);
    }
  }

  for (uint8_t cross = 0; cross < 2; cross++)
    for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++)
      for (uint8_t step = 0; step < MAXSTEPS; step++)
        playingPatterns[cross][instr][step] = 0;

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
  oledWelcome();
  display.display();

  //internal clock
  u_bpm = bpm2micros4ppqn(bpm);
  Timer3.attachInterrupt(ISRfireTimer3);
  Timer3.start(u_bpm);
  Timer4.attachInterrupt(ISRfireTimer4);
  Timer5.attachInterrupt(endTriggers);
}

//base clock
void ISRfireTimer3()
{
  Timer3.stop();
  u_tickInterval = micros() - u_lastTick;
  u_lastTick = micros();
  if (u_timeShift >= 0)
    Timer4.start(u_timeShift);
  else
    Timer4.start(u_tickInterval + u_timeShift);
  Timer3.start(bpm2micros4ppqn(bpm));
}

//shifted clock and everything to step sequencer related
void ISRfireTimer4()
{
  noInterrupts();
  Timer5.start(DEFAULTGATELENGHT);                         //start 5ms first trigger timer
  Timer4.stop();                                           //stop timer shift
  uint8_t playMidiDeck[2] = {0, 0};                        //calculate final noteVelocity
  for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++) //for each instrument
  {
    uint8_t targetDeck = previousDeck; //force point to previous deck
    if ((crossfader - 1) >= instr)     //if instrument already crossed
      targetDeck = thisDeck;           // point to this deck
    //if not permanently muted and not momentary muted and it is a valid step
    if (!permanentMute[instr] && !instrActionState[instr] && playingPatterns[targetDeck][instr][stepCount])
    {
      digitalWrite(triggerPins[instr], HIGH);
      playMidiDeck[targetDeck] = playMidiDeck[targetDeck] + powint(2, 5 - instr); //accumulate to play this or previous instrument
    }
  }
  if (playMidiDeck[thisDeck] != 0)
    playMidi(thisDeck + 1, playMidiDeck[thisDeck], MIDICHANNEL); //send midinote to this deck with binary coded trigger message
  if (playMidiDeck[previousDeck] != 0)
    playMidi(previousDeck + 1, playMidiDeck[previousDeck], MIDICHANNEL); //send midinote to previous deck with binary coded trigger message
  interrupts();
  gateLenghtCounter = 0;
  stepCount++;
  if (stepCount >= MAXSTEPS)
    stepCount = 0;
  sampleChangeWindowEndTime = (micros() + u_tickInterval - OLEDUPDATETIME);
}

void loop()
{
  //flags if any Oled update will be necessary in this cycle
  boolean updateOled = false;

  //read all processed encoders interruptions
  queuedEncoder++;
  if (queuedEncoder == MAXENCODERS)
    queuedEncoder = 0;
  readEncoder(queuedEncoder);

  //check for all Oled updates ===============================================================
  //if new mood was selected
  if (updateOledBrowseMood)
  {
    checkDefaultOled();
    oledShowBrowsedMood();
    updateOledBrowseMood = false;
    updateOled = true;
  }
  //update right upper corner with bpm value
  else if (updateOledUpdateBpm)
  {
    updateOledUpdateBpm = false;
    oledShowCornerInfo(2, bpm);
    updateOled = true;
  }
  //update cross bar changes and there is available time to load new sample and play it
  //this could be took off if using Tsunami
  else if ((updateOledUpdateCross != 0) && sampleUpdateWindow())
  {
    checkDefaultOled();
    oledShowCrossBar(crossfader);
    updateOledUpdateCross = 0;
    updateOled = true;
  }
  //copy selected mood to new deck
  else if (updateOledSelectMood && sampleUpdateWindow())
  {
    //before load new mood, prepare current deck to leave it the same way user left
    for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++)
    {
      //if there are instruments not morphed in the current deck , or silenced then kill any reference
      if ((instr >= crossfader) || (permanentMute[instr] == 1))
      {
        for (uint8_t step = 0; step < MAXSTEPS; step++)
          playingPatterns[thisDeck][instr][step]; //kill all playing pattern steps
        deckSamples[thisDeck][instr]->reset();         //kill all samples pointers
        deckPatterns[thisDeck][instr]->reset();        //kill all patterns pointers
      }
    }

    //change decks
    thisDeck = !thisDeck;
    previousDeck = !previousDeck;

    //reset any customization
    checkDefaultOled();
    resetAllPermanentMute();                    //in common
    resetAllGateLenght();                       //in common
    restoreCustomizedPatternsToOriginalRef(); //in common
    setAllPatternAsOriginal();                  //in common
    for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    {
      deckSamples[thisDeck][i]->setValue(moodKitRefPointers[selectedMood][i + MAXINSTRUMENTS]);              //load all samples
      deckPatterns[thisDeck][i]->setValue(moodKitRefPointers[selectedMood][i + MAXINSTRUMENTS]);             //load all patterns
      copyRefPatternToThisDeckPlayingPatterns(i, deckPatterns[thisDeck][i]->getValue());                     //copy selected instrument pattern to this deck playing
      playMidi(instrSampleMidiNote[i] + (10 * thisDeck), deckSamples[thisDeck][i]->getValue(), MIDICHANNEL); //send MIDI to pi
    }
    oledShowPreviousMood();
    oledShowSelectedMood();
    previousMood = selectedMood;
    oledShowCrossBar(-1);
    crossfader = 0;
    updateOledSelectMood = false;
    updateOled = true;
  }

  //if only one intrument pattern changed
  else if (updateOledInstrPattern != -1)
  {
    oledShowInstrumPattern(updateOledInstrPattern, RAM);
    oledShowCornerInfo(0, deckPatterns[thisDeck][updateOledInstrPattern]->getValue());
    oledCustomMoodName();
    updateOled = true;
    updateOledInstrPattern = -1;
  }
  //if sampler changed and there is available time to update screen
  else if ((updateOledChangePlayingSample != -1) && sampleUpdateWindow())
  {
    playMidi(instrSampleMidiNote[updateOledChangePlayingSample] + (10 * thisDeck), deckSamples[thisDeck][updateOledChangePlayingSample]->getValue(), MIDICHANNEL);
    oledCustomMoodName();
    oledShowCornerInfo(1, deckSamples[thisDeck][updateOledChangePlayingSample]->getValue());
    updateOled = true;
    updateOledChangePlayingSample = -1;
  }

  //if time shift changed
  else if (updateOledUpdateTimeShift)
  {
    oledShowCornerInfo(3, (u_timeShift - 100) / 1000);
    updateOled = true;
    updateOledUpdateTimeShift = false;
  }
  //if gate lenght changed
  else if (updateOledUpdategateLenght != -1)
  {
    oledShowCornerInfo(4, updateOledUpdategateLenght);
    updateOled = true;
    updateOledUpdategateLenght = -1;
  }
  //if erase pattern
  else if (updateOledEraseInstrumentPattern != -1)
  {
    oledEraseInstrumentPattern(updateOledEraseInstrumentPattern); //clear pattern from display
    updateOled = true;
    updateOledEraseInstrumentPattern = -1;
  }
  //if step should be tapped
  else if (updateOledTapInstrumentPattern != -1)
  {
    oledShowInstrumPattern(updateOledTapInstrumentPattern, RAM); //update Oled with new inserted step
    updateOled = true;
    updateOledTapInstrumentPattern = -1;
  }
  else if (updateOledRollbackInstrumentTap != -1)
  {
    oledShowInstrumPattern(updateOledRollbackInstrumentTap, RAM); //update Oled with removed step
    updateOled = true;
    updateOledRollbackInstrumentTap = -1;
  }

  //if any Oled update
  if (updateOled)
    display.display();

  //read interface inputs ===========================================================================================================
  //read all encoders buttons (invert boolean state to make easy future comparation)
  for (int8_t i = 0; i < MAXENCODERS; i++)
    encoderButtonState[i] = !digitalRead(encoderButtonPins[i]);

  //if new mood was selected....copy reference tables or create new mood in the cross area
  if (onlyOneEncoderButtonIsPressed(ENCBUTMOOD) && interfaceEvent.debounced())
  {
    interfaceEvent.debounce(1000); //block any other interface event
    updateOledSelectMood = true;
  }

  //read all action buttons (invert boolean state to make easy future comparation)
  for (int8_t i = 0; i < MAXINSTRUMENTS; i++)
  {
    instrActionState[i] = !digitalRead(instrActionPins[i]); //read action instrument button

    //if any pattern should be erased
    //cross encoder + instrument modifier + action button + not custom pattern
    if (twoEncoderButtonsArePressed(CROSSENCODER, i + 2) && instrActionState[i] && !isCustomPattern[i] && interfaceEvent.debounced())
    {
      //if this pattern wasnt BKPd yet
      if (customizedPattern[i] == 0)
      {
        copyRefPatternToRefPattern(i, deckPatterns[thisDeck][i]->getValue(), BKPPATTERN); //save actual pattern into bkp area 0
        customizedPattern[i] = deckPatterns[thisDeck][i]->getValue();                     //save actual playing pattern as BKPd
        setThisPatternAsCustom(i);
      }
      copyRefPatternToRefPattern(i, 1, deckPatterns[thisDeck][i]->getValue()); //copy empty pattern to deck 1
      eraseInstrumentRam1Pattern(i);
      interfaceEvent.debounce(1000);
      updateOledEraseInstrumentPattern = i;
    }
    //if any tap should be rollbacked
    //cross encoder + instrument modifier + action button + custom pattern
    else if (twoEncoderButtonsArePressed(CROSSENCODER, i + 2) && instrActionState[i] && isCustomPattern[i] && interfaceEvent.debounced())
    {
      //if there was any rollback available
      if (undoStack[0][i] != -1)
      {
        setStepIntoRefPattern(i, deckPatterns[thisDeck][i]->getValue(), undoStack[0][i], 0); //insert new step into Rom pattern
        setStepIntoplayingPattern(i, undoStack[0][i], 0);                                    //insert new step into Ram 1 pattern
        rollbackStepIntoUndoArray(i);                                                        //rollback the last saved step
        oledShowInstrumPattern(i, RAM);                                                      //update Oled with new inserted step
        updateOled = true;
        updateOledRollbackInstrumentTap = i;
        interfaceEvent.debounce(200);
      }
    }
    //if any pattern should be permanently muted
    //cross encoder + action button
    else if (onlyOneEncoderButtonIsPressed(CROSSENCODER) && instrActionState[i] && interfaceEvent.debounced())
    {
      permanentMute[i] = !permanentMute[i];
      interfaceEvent.debounce(1000);
    }
    //if any pattern should be tapped
    //one instrument modifier + action button
    else if (onlyOneEncoderButtonIsPressed(i + 2) && instrActionState[i] && interfaceEvent.debounced())
    {
      int8_t updateOledTapStep;
      if (micros() < (u_lastTick + (u_tickInterval >> 1))) //if we are still in the same step
      {
        //tap this step
        updateOledTapStep = stepCount;
        playMidi(1, powint(2, 5 - i), MIDICHANNEL); //send midinote to play only this instrument
      }
      else
      {
        //tap saved to next step
        int8_t nextStep = stepCount;
        nextStep++;
        if (nextStep >= MAXSTEPS)
          nextStep = 0;
        updateOledTapStep = nextStep;
        //no need to send midinote to play this instrument because it will be saved to next step cycle
      }
      //if this pattern isnt bkp´d yet
      if (customizedPattern[i] == 0)
      {
        copyRefPatternToRefPattern(i, deckPatterns[thisDeck][i]->getValue(), BKPPATTERN); //save actual pattern into bkp area 0
        customizedPattern[i] = deckPatterns[thisDeck][i]->getValue();                     //save actual playing pattern as BKPd
      }
      setStepIntoRefPattern(i, deckPatterns[thisDeck][i]->getValue(), updateOledTapStep, 1); //insert new step into Rom pattern
      setStepIntoplayingPattern(i, updateOledTapStep, 1);                                    //insert new step into Ram 1 pattern
      setStepIntoUndoArray(i, updateOledTapStep);
      setThisPatternAsCustom(i);
      updateOledTapInstrumentPattern = i;
      interfaceEvent.debounce(u_tickInterval / 1000);
    }
  }
}

//allows to change samples only upo to after 2/3 of the tick interval
//to avoid to change sample the same time it was triggered
boolean sampleUpdateWindow()
{
  return (micros() < sampleChangeWindowEndTime);
}

//close all trigger
//  _ ______ _______ _______ _______
//_| |      |       |       |       |______
void endTriggers()
{
  //compare all triggers times
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
  {
    if (gateLenghtSize[i] <= gateLenghtCounter)
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
        updateOledBrowseMood = true;
        lastSelectedMood = selectedMood;
      }
      break;

    case CROSSENCODER:
      //if cross button AND cross button are pressed and cross rotate change timer shift
      if (twoEncoderButtonsArePressed(ENCBUTMOOD, ENCBUTCROSS))
      {
        u_timeShift += encoderChange * u_timeShiftStep;
        u_timeShift = constrain(u_timeShift, -u_timeShiftLimit, u_timeShiftLimit);
        updateOledUpdateTimeShift = true;
      }
      //if cross button is pressed and cross rotate CHANGE BPM
      else if (onlyOneEncoderButtonIsPressed(ENCBUTCROSS))
      {
        updateOledUpdateBpm = true;
        bpm += encoderChange;
        u_bpm = bpm2micros4ppqn(bpm);
      }
      else
      { //or else , cross change
        crossfader += encoderChange;
        crossfader = constrain(crossfader, 0, MAXINSTRUMENTS);
        updateOledUpdateCross = encoderChange;
      }
      break;
    default:
      //all other instrument encoders
      uint8_t _instrum = _queued - 2;
      //if only one ASSOCIATED modifier pressed
      if (onlyOneEncoderButtonIsPressed(sampleButtonModifier[_queued]))
      {
        if (encoderChange == -1)
          deckSamples[thisDeck][_instrum]->reward();
        else
          deckSamples[thisDeck][_instrum]->advance();
        // if ((_instrum) <= (crossfader))
        // else
        //   deckPointers[0][_instrum] = actualSample[_instrum];
        updateOledChangePlayingSample = _instrum;
      }
      //if only SAME button pressed , change gate lenght
      else if (onlyOneEncoderButtonIsPressed(_queued))
      {
        gateLenghtSize[_instrum] += encoderChange;
        gateLenghtSize[_instrum] = constrain(gateLenghtSize[_instrum], 0, MAXGATELENGHTS);
        updateOledUpdategateLenght = _instrum;
      }
      //if no modifier pressed , only change instrument pattern , schedule event
      else if (noOneEncoderButtonIsPressed())
      {
        //if this pattern was tapped and BKPd , restore
        if (customizedPattern[_instrum] != 0)
        {
          copyRefPatternToRefPattern(_instrum, BKPPATTERN, deckPatterns[thisDeck][_instrum]->getValue()); //reverse BKPd pattern to rom area
          clearUndoArray(_instrum);
          setThisPatternAsOriginal(_instrum);
        }
        customizedPattern[_instrum] = 0;
        if (encoderChange == -1)
          deckPatterns[thisDeck][_instrum]->reward();
        else
          deckPatterns[thisDeck][_instrum]->advance();
        copyRefPatternToThisDeckPlayingPatterns(_instrum, deckPatterns[thisDeck][_instrum]->getValue()); //copy selected instrument pattern to RAM
        updateOledInstrPattern = _instrum;                                                               //flags to update this instrument on Display
      }
      break;
    }
  }
}

//check if default Oled wasnt show yet
void checkDefaultOled()
{
  if (defaultOledNotActiveYet)
  {
    defaultOledNotActiveYet = false;
    display.clearDisplay();
    display.drawRect(0, 0, 60, TEXTLINE_HEIGHT - 2, WHITE);
    oledShowBrowsedMood();
    oledShowCrossBar(-1);
  }
}

void oledWelcome()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("<-- select your mood"));
  display.println(F("    and cross it -->"));
}

void oledShowCrossBar(int8_t _size) //update crossing status / 6 steps of 10 pixels each
{
  if (lastCrossBarGraphValue != _size)
  {
    display.fillRect(1, 1, 58, TEXTLINE_HEIGHT - 4, BLACK);
    for (int8_t i = 0; i < _size; i++)
      display.fillRect(10 * i, 0, 10, TEXTLINE_HEIGHT - 2, WHITE);
    lastCrossBarGraphValue = _size;
  }
}

void oledShowCornerInfo(uint8_t _parm, int16_t _val) //update Oled right upper corner with the actual sample or pattern number
{
  display.fillRect(70, 0, DISPLAY_WIDTH - 70, TEXTLINE_HEIGHT, BLACK);
  display.setCursor(70, 0);
  display.setTextColor(WHITE);
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
    display.print((DEFAULTGATELENGHT + (gateLenghtSize[_val] * EXTENDEDGATELENGHT)) / 1000);
  }
}

void oledShowPreviousMood()
{
  display.fillRect(0, TEXTLINE_HEIGHT, DISPLAY_WIDTH, TEXTLINE_HEIGHT, BLACK);
  display.setTextColor(WHITE);
  display.setCursor(0, TEXTLINE_HEIGHT);    //set cursor position
  display.print(moodKitName[previousMood]); //print previous mood name
}

void oledShowSelectedMood()
{
  display.fillRect(0, 2 * TEXTLINE_HEIGHT, DISPLAY_WIDTH, TEXTLINE_HEIGHT, BLACK);
  display.setTextColor(WHITE);
  display.setCursor(0, 2 * TEXTLINE_HEIGHT); //set cursor position
  display.print(moodKitName[selectedMood]);  //print selected mood name
}
void oledCustomMoodName()
{
  oledEraseBrowsedMoodName();
  display.setTextColor(WHITE);
  display.setCursor(0, 3 * TEXTLINE_HEIGHT); //set cursor position
  display.print("Custom");                   //print Custom name
}
void oledEraseBrowsedMoodName()
{
  display.fillRect(0, 3 * TEXTLINE_HEIGHT, DISPLAY_WIDTH, TEXTLINE_HEIGHT - 1, BLACK);
}

void oledShowBrowsedMood() //update almost all bottom Oled area with the name of the selected mood and all 6 available instruments
{
  oledEraseBrowsedMoodName();
  display.setTextColor(WHITE);
  display.setCursor(0, 3 * TEXTLINE_HEIGHT);              //set cursor position
  display.print(moodKitName[selectedMood]);               //print mood name
  for (int8_t instr = 0; instr < MAXINSTRUMENTS; instr++) //for each instrument
    oledShowInstrumPattern(instr, ROM);
}

void oledShowInstrumPattern(uint8_t _instr, boolean _source)
{
  boolean mustPrintStep = false;
  uint8_t referencePattern = moodKitRefPointers[selectedMood][_instr + MAXINSTRUMENTS];
  oledEraseInstrumentPattern(_instr);
  display.setTextColor(WHITE);
  for (int8_t step = 0; step < (MAXSTEPS - 1); step++) //for each step
  {
    mustPrintStep = false;
    //source = customized pattern and it is a valid pattern
    if ((_source == RAM) && (playingPatterns[thisDeck][_instr][step])) //if source = RAM
    {
      mustPrintStep = true;
    }
    //source = reference patterns and a valid pattern
    else if ((_source == ROM) && (referencePattern > 0))
    {
      if (_instr == KICKCHANNEL)
      {
        if (refPattern1[referencePattern][step]) //if it is an active step
          mustPrintStep = true;
      }
      else if (_instr == SNAKERCHANNEL)
      {
        if (refPattern2[referencePattern][step]) //if it is an active step
          mustPrintStep = true;
      }
      else if (_instr == CLAPCHANNEL)
      {
        if (refPattern3[referencePattern][step]) //if it is an active step
          mustPrintStep = true;
      }
      else if (_instr == HATCHANNEL)
      {
        if (refPattern4[referencePattern][step]) //if it is an active step
          mustPrintStep = true;
      }
      else if (_instr == OHHRIDECHANNEL)
      {
        if (refPattern5[referencePattern][step]) //if it is an active step
          mustPrintStep = true;
      }
      else
      {
        if (refPattern6[referencePattern][step]) //if it is an active step
          mustPrintStep = true;
      }
    }
    if (mustPrintStep)
      display.fillRect(step * 2, DOTGRIDINIT + (_instr * GRIDPATTERNHEIGHT), 2, GRIDPATTERNHEIGHT - 1, WHITE);
  }
}

//erase exatctly one line pattern
void oledEraseInstrumentPattern(uint8_t _instr)
{
  display.fillRect(0, DOTGRIDINIT + (_instr * GRIDPATTERNHEIGHT), DISPLAY_WIDTH, GRIDPATTERNHEIGHT - 1, BLACK);
}

void playMidi(uint8_t _note, uint8_t _velocity, uint8_t _channel)
{
#ifdef DO_MIDIHW
  MIDI.sendNoteOn(_note, _velocity, _channel);
#endif

#ifdef DO_MIDIUSB
  noteOn(_channel, _note, _velocity); // Channel 0, middle C, normal velocity
  MidiUSB.flush();
#endif
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

void resetAllGateLenght()
{
  for (uint8_t pat = 0; pat < MAXINSTRUMENTS; pat++)
    gateLenghtSize[pat] = 0; //clear all gate lengh sizes
}

void resetAllPermanentMute()
{
  for (uint8_t pat = 0; pat < MAXINSTRUMENTS; pat++)
    permanentMute[pat] = 0;
}

void restoreCustomizedPatternsToOriginalRef()
{
  for (uint8_t pat = 0; pat < MAXINSTRUMENTS; pat++) // search for any BKPd pattern
  {
    if (customizedPattern[pat] != 0) //if any pattern was customized , restore it from bkp before do anything
    {
      copyRefPatternToRefPattern(pat, BKPPATTERN, customizedPattern[pat]); //restore BKPd pattern to its original place
      customizedPattern[pat] = 0;
    }
  }
}

//clear pattern from ram 1 area
void eraseInstrumentRam1Pattern(uint8_t _instr)
{
  for (int8_t step = 0; step < MAXSTEPS; step++)
    playingPatterns[thisDeck][_instr][step] = 0;
}

//save tapped step into respective rom table
void setStepIntoRefPattern(uint8_t _instr, uint8_t _pattern, uint8_t _step, uint8_t _val)
{
  if (_instr == KICKCHANNEL)
    refPattern1[_pattern][_step] = _val;
  else if (_instr == SNAKERCHANNEL)
    refPattern2[_pattern][_step] = _val;
  else if (_instr == CLAPCHANNEL)
    refPattern3[_pattern][_step] = _val;
  else if (_instr == HATCHANNEL)
    refPattern4[_pattern][_step] = _val;
  else if (_instr == OHHRIDECHANNEL)
    refPattern5[_pattern][_step] = _val;
  else
    refPattern6[_pattern][_step] = _val;
}

void setStepIntoplayingPattern(uint8_t _instr, uint8_t _step, uint8_t _val)
{
  playingPatterns[thisDeck][_instr][_step] = _val; //save tapped step
}

void clearUndoArray(uint8_t _instr)
{
  for (uint8_t i = 0; i < MAXUNDOS; i++)
    undoStack[i][_instr] = -1;
}

void setStepIntoUndoArray(uint8_t _instr, uint8_t _step)
{
  for (uint8_t i = (MAXUNDOS - 1); i > 0; i--)
    undoStack[i][_instr] = undoStack[i - 1][_instr]; //move all values ahead to open space on stack
  undoStack[0][_instr] = _step;
}

void setAllPatternAsOriginal()
{
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    setThisPatternAsOriginal(i);
}

void setThisPatternAsOriginal(uint8_t _instr)
{
  isCustomPattern[_instr] = 0;
}

void setThisPatternAsCustom(uint8_t _instr)
{
  isCustomPattern[_instr] = 1;
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
void copyRefPatternToThisDeckPlayingPatterns(uint8_t _instr, uint8_t _romPatternTablePointer)
{
  //code repeat protection
  if ((lastCopied_instr == _instr) && (lastCopied_PatternTablePointer == _romPatternTablePointer))
    return;
  lastCopied_instr = _instr;
  lastCopied_PatternTablePointer = _romPatternTablePointer;
  for (uint8_t i = 0; i < MAXSTEPS; i++)
  {
    if (_instr == KICKCHANNEL)
      playingPatterns[thisDeck][_instr][i] = refPattern1[_romPatternTablePointer][i];
    else if (_instr == SNAKERCHANNEL)
      playingPatterns[thisDeck][_instr][i] = refPattern2[_romPatternTablePointer][i];
    else if (_instr == CLAPCHANNEL)
      playingPatterns[thisDeck][_instr][i] = refPattern3[_romPatternTablePointer][i];
    else if (_instr == HATCHANNEL)
      playingPatterns[thisDeck][_instr][i] = refPattern4[_romPatternTablePointer][i];
    else if (_instr == OHHRIDECHANNEL)
      playingPatterns[thisDeck][_instr][i] = refPattern5[_romPatternTablePointer][i];
    else
      playingPatterns[thisDeck][_instr][i] = refPattern6[_romPatternTablePointer][i];
  }
}

//copy selected instrument pattern from ROM to ROM
void copyRefPatternToRefPattern(uint8_t _instr, uint8_t _source, uint8_t _target)
{
  for (uint8_t i = 0; i < MAXSTEPS; i++)
  {
    if (_instr == KICKCHANNEL)
      refPattern1[_target][i] = refPattern1[_source][i];
    else if (_instr == SNAKERCHANNEL)
      refPattern2[_target][i] = refPattern2[_source][i];
    else if (_instr == CLAPCHANNEL)
      refPattern3[_target][i] = refPattern3[_source][i];
    else if (_instr == HATCHANNEL)
      refPattern4[_target][i] = refPattern4[_source][i];
    else if (_instr == OHHRIDECHANNEL)
      refPattern5[_target][i] = refPattern5[_source][i];
    else
      refPattern6[_target][i] = refPattern6[_source][i];
  }
}

//copy ramPattern form src area to trg
void copyRamPatternToRamPattern(uint8_t _src, uint8_t _trg, uint8_t _instr)
{
  for (uint8_t i = 0; i < MAXSTEPS; i++)
    playingPatterns[_trg][_instr][i] = playingPatterns[_src][_instr][i];
}

#ifdef DO_MIDIUSB
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

void controlChange(byte channel, byte control, byte value)
{
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}
#endif