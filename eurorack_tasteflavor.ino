/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create melodies
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#define DO_SERIAL true
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
#include <EventDebounce.h>
#include <Rotary.h>
#include <DueTimer.h>
#include "Deck.h"

Deck *deck[2];
Patterns *pattern;

#define I2C_ADDRESS 0x3C
#define DISPLAY_WIDTH 128 // OLED Oled width, in pixels
#define DISPLAY_HEIGHT 64 // OLED Oled height, in pixels
#define TEXTLINE_HEIGHT 9 // OLED Oled height, in pixels

//adafruit
#include <Adafruit_SSD1306.h>
#define OLED_RESET 47 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, OLED_RESET);

//oled
#define OLEDUPDATETIME 35000 //microsseconds to update display
#define DOTGRIDINIT 36
#define GRIDPATTERNHEIGHT 4

//sequencer
#define MAXSTEPS 64      //max step sequence
#define MYCHANNEL 1      //midi channel for data transmission
#define MAXENCODERS 8    //total of encoders
#define MOODENCODER 0    //MOOD encoder id
#define CROSSENCODER 1   //CROSS encoder id
#define MAXINSTRUMENTS 6 //total of instruments
#define MIDICHANNEL 1    //standart drum midi channel
#define MOODMIDINOTE 20  //midi note to mood message
#define CROSSMIDINOTE 21 //midi note to cross message

#define KICKCHANNEL 0
#define SNAKERCHANNEL 1 //snare + shaker
#define CLAPCHANNEL 2
#define HATCHANNEL 3
#define OHHRIDECHANNEL 4 //open hi hat + ride
#define PERCCHANNEL 5

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

//sequencer
volatile int8_t stepCount = 0;
int8_t queuedEncoder = 0;

//mood
#define MAXMOODS 4 //max moods
String moodKitName[MAXMOODS] = {"", "P.Labs-Choke", "P.Labs-Empty Room", "P.Labs-April23"};
uint8_t moodKitPatterns[MAXMOODS][MAXINSTRUMENTS] = {
    {1, 1, 1, 1, 1, 1}, //reserved MUTE = 1
    {2, 3, 4, 2, 4, 1}, //P.Labs-Choke
    {2, 2, 2, 2, 2, 1}, //P.Labs-Empty Room
    {2, 3, 3, 3, 3, 1}  //P.Labs-April23
};

//decks
#define RAM 0
#define ROM 1
int8_t crossfader = 0;

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
void oledShowInstrPattern(uint8_t _instr, boolean _src);
void oledShowPreviousMood();
void oledCustomMoodName();
void oledShowSelectedMood();
void endTriggers();

// boolean crossfadedDeck(uint8_t _instr);
// boolean sampleUpdateWindow();
// boolean noOneEncoderButtonIsPressed();
// boolean onlyOneEncoderButtonIsPressed(uint8_t _target);
// boolean twoEncoderButtonsArePressed(uint8_t _target1, uint8_t _target2);

void setup()
{
#ifdef DO_MIDIHW
  MIDI.begin();
#endif

#ifdef DO_SERIAL
  Serial.begin(9600);
  Serial.println("Debugging..");
#endif

  deck[0] = new Deck();
  deck[1] = new Deck();
  pattern = new Patterns();

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
  {
    pinMode(triggerPins[i], OUTPUT);
    digitalWrite(triggerPins[i], LOW);
  }
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
  {
    digitalWrite(triggerPins[i], HIGH);
    delay(300);
  }
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
  {
    digitalWrite(triggerPins[i], LOW);
    delay(300);
  }

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
  uint8_t playMidiDeck[2] = {0, 0};                        //calculate final noteVelocity
  for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++) //for each instrument
  {
    boolean targetDeck;
    targetDeck = crossfadedDeck(instr);
    if (!deck[targetDeck]->permanentMute[instr]                                                                             //
        && !instrActionState[instr]                                                                                         //
        && deck[targetDeck]->pattern->isThisStepActive(instr, deck[targetDeck]->deckPatterns[instr]->getValue(), stepCount) //
    )
    {
      digitalWrite(triggerPins[instr], HIGH);
      playMidiDeck[targetDeck] = playMidiDeck[targetDeck] + powint(2, 5 - instr); //acumulate to play this or previous instrument
    }
  }
  if (playMidiDeck[thisDeck] != 0)
    playMidi(thisDeck + 1, playMidiDeck[thisDeck], MIDICHANNEL); //send midinote to this deck with binary coded trigger message
  if (playMidiDeck[!thisDeck] != 0)
    playMidi(!thisDeck + 1, playMidiDeck[!thisDeck], MIDICHANNEL); //send midinote to other deck with binary coded trigger message
  interrupts();
  gateLenghtCounter = 0;
  stepCount++;
  if (stepCount >= MAXSTEPS)
    stepCount = 0;
  sampleChangeWindowEndTime = (micros() + u_tickInterval - OLEDUPDATETIME);
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

    if (_queued == MOODENCODER)
    {
      selectedMood += encoderChange;
      selectedMood = constrain(selectedMood, 0, MAXMOODS - 1);
      if (lastSelectedMood != selectedMood)
      {
        updateOledBrowseMood = true;
        lastSelectedMood = selectedMood;
      }
    }
    else if (_queued == CROSSENCODER)
    {
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
    }
    else
    {
      //all other instrument encoders
      uint8_t _instrum = _queued - 2;
      //if only one ASSOCIATED modifier pressed
      if (onlyOneEncoderButtonIsPressed(sampleButtonModifier[_queued]))
      {
        if (encoderChange == -1)
          deck[thisDeck]->deckSamples[_instrum]->reward();
        else
          deck[thisDeck]->deckSamples[_instrum]->advance();
        updateOledChangePlayingSample = _instrum;
      }
      //if only SAME button pressed , change gate lenght
      else if (onlyOneEncoderButtonIsPressed(_queued))
      {
        deck[thisDeck]->changeGateLenghSize(_instrum, encoderChange);
        updateOledUpdategateLenght = _instrum;
      }
      //nothing pressed, only pattern change , schedule event
      else if (noOneEncoderButtonIsPressed())
      {
        if (encoderChange == -1)
          deck[thisDeck]->deckPatterns[_instrum]->reward();
        else
          deck[thisDeck]->deckPatterns[_instrum]->advance();
        updateOledInstrPattern = _instrum; //flags to update this instrument on Display
      }
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
  String rightCornerInfo[5] = {"pat", "smp", "bpm", "ms", "len"};
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
    display.print((DEFAULTGATELENGHT + (deck[crossfadedDeck(_val)]->gateLenghtSize[_val] * EXTENDEDGATELENGHT)) / 1000);
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

uint8_t lastBrowsedMood;
void oledShowBrowsedMood() //update almost all bottom Oled area with the name of the selected mood and all 6 available instruments
{
  if (lastBrowsedMood != selectedMood)
  {
    lastBrowsedMood = selectedMood;
    oledEraseBrowsedMoodName();
    display.setTextColor(WHITE);
    display.setCursor(0, 3 * TEXTLINE_HEIGHT);               //set cursor position
    display.print(moodKitName[selectedMood]);                //print mood name
    for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++) //for each instrument
      oledShowInstrPattern(instr, ROM);
  }
}

void oledShowInstrPattern(uint8_t _instr, boolean _src)
{
  oledEraseInstrumentPattern(_instr);
  display.setTextColor(WHITE);
  for (int8_t step = 0; step < (MAXSTEPS - 1); step++) //for each step
  {
    //if browsed mood (ROM) or individual pattern browse (RAM)
    if (((_src == ROM) && (pattern->getStep(_instr, moodKitPatterns[selectedMood][_instr], step))) ||
        ((_src == RAM) && (deck[thisDeck]->pattern->getStep(_instr, deck[thisDeck]->deckPatterns[_instr]->getValue(), step))))
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
  //mood browse
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
    checkDefaultOled();
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
    oledShowInstrPattern(updateOledInstrPattern, RAM);
    oledShowCornerInfo(0, deck[thisDeck]->deckPatterns[updateOledInstrPattern]->getValue());
    oledCustomMoodName();
    updateOled = true;
    updateOledInstrPattern = -1;
  }
  //if sampler changed and there is available time to update screen
  else if ((updateOledChangePlayingSample != -1) && sampleUpdateWindow())
  {
    playMidi(instrSampleMidiNote[updateOledChangePlayingSample] + (10 * thisDeck), deck[thisDeck]->deckSamples[updateOledChangePlayingSample]->getValue(), MIDICHANNEL);
    oledCustomMoodName();
    oledShowCornerInfo(1, deck[thisDeck]->deckSamples[updateOledChangePlayingSample]->getValue());
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
    oledShowInstrPattern(updateOledTapInstrumentPattern, RAM); //update Oled with new inserted step
    updateOled = true;
    updateOledTapInstrumentPattern = -1;
  }
  else if (updateOledRollbackInstrumentTap != -1)
  {
    oledShowInstrPattern(updateOledRollbackInstrumentTap, RAM); //update Oled with removed step
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
    //before load new mood, prepare current deck to leave it the same way user customized
    for (uint8_t instr = 0; instr < MAXINSTRUMENTS; instr++)
      //if there were some instrument not morphed in the current deck then mute it
      if (crossfadedDeck(instr) == !thisDeck)
        // if (instr >= crossfader)
        deck[thisDeck]->permanentMute[instr] = true;

    //change decks
    thisDeck = !thisDeck;
    deck[thisDeck]->cue(selectedMood,                     //
                        moodKitPatterns[selectedMood][0], //
                        moodKitPatterns[selectedMood][1], //
                        moodKitPatterns[selectedMood][2], //
                        moodKitPatterns[selectedMood][3], //
                        moodKitPatterns[selectedMood][4], //
                        moodKitPatterns[selectedMood][5]  //
    );
    //send MIDI to pi to load sample
    for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
      playMidi(instrSampleMidiNote[i] + (10 * thisDeck), moodKitPatterns[selectedMood][i], MIDICHANNEL);

    updateOledSelectMood = true;
    interfaceEvent.debounce(1000); //block any other interface event
  }

  //read all action buttons (invert boolean state to make easy future comparation)
  for (int8_t i = 0; i < MAXINSTRUMENTS; i++)
  {
    instrActionState[i] = !digitalRead(instrActionPins[i]); //read action instrument button

    //if any tap should be rollbacked
    //cross encoder + instrument modifier + action button + custom pattern
    if (twoEncoderButtonsArePressed(CROSSENCODER, i + 2)   //
        && instrActionState[i]                             //
        && deck[thisDeck]->pattern->isThisCustomPattern(i) //
        && interfaceEvent.debounced())
    {
      //if there was any rollback available
      if (deck[thisDeck]->pattern->undoAvailable(i))
      {
        deck[thisDeck]->pattern->rollbackUndoStep(i, deck[thisDeck]->deckPatterns[i]->getValue()); //rollback the last saved undo
        oledShowInstrPattern(i, RAM);                                                              //update Oled with new inserted step
        updateOled = true;
        updateOledRollbackInstrumentTap = i;
        interfaceEvent.debounce(200);
      }
    }
    //if any pattern should be erased
    //cross encoder + instrument modifier + action button + not custom pattern
    else if (twoEncoderButtonsArePressed(CROSSENCODER, i + 2) && instrActionState[i] && interfaceEvent.debounced())
    {
      //erase pattern IF possible
      deck[thisDeck]->pattern->eraseInstrumentPattern(i, deck[thisDeck]->deckPatterns[i]->getValue());
      interfaceEvent.debounce(1000);
      updateOledEraseInstrumentPattern = i;
    }
    //if any pattern should be permanently muted
    //cross encoder + action button
    else if (onlyOneEncoderButtonIsPressed(CROSSENCODER) && instrActionState[i] && interfaceEvent.debounced())
    {
      deck[thisDeck]->permanentMute[i] = deck[thisDeck]->permanentMute[i];
      interfaceEvent.debounce(1000);
    }
    //if any pattern should be tapped
    //one instrument modifier + action button
    else if (onlyOneEncoderButtonIsPressed(i + 2) && instrActionState[i] && interfaceEvent.debounced())
    {
      //calculate if the tap will be saved on this step or into next
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
      deck[thisDeck]->pattern->tapStep(i, deck[thisDeck]->deckPatterns[i]->getValue(), updateOledTapStep);
      updateOledTapInstrumentPattern = i;
      interfaceEvent.debounce(u_tickInterval / 1000);
    }
  }
}
