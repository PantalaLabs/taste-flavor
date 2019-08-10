
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();

#include <PantalaDefines.h>
#include <Switch.h>
#include <MicroDebounce.h>
#include <EventDebounce.h>
#include <Counter.h>
#include <Trigger.h>

#define MAXCOMBINATIONS 28   //max steps combinations
#define MAXSTEPS 32          //max step sequence
#define MYCHANNEL 1          //midi channel for data transmission
#define MAXENCODERS 8        //total of encoders
#define MOODENCODER 0        //total of sample encoders
#define MORPHENCODER 1       //total of sample encoders
#define FIRSTSAMPLEENCODER 2 //total of sample encoders
#define MAXINSTRUMENTS 6     //total of sample encoders
#define TRIGGERINPIN 7       //total of sample encoders

#define MIDICHANNEL 1
uint8_t INSTRmidiNote[MAXINSTRUMENTS] = {10, 11, 12, 13, 14, 15};
#define MOODMIDINOTE 20
#define MORPHMIDINOTE 21
EventDebounce stepInterval(125);

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

Switch triggerIn(TRIGGERINPIN);
Counter stepCounter(MAXSTEPS - 1);
Counter encoderQueue(MAXENCODERS - 1);

// #define SUBCHANNEL 6
// #define KICKCHANNEL 4
// #define SNARECHANNEL 2
// #define CLAPCHANNEL 17
// #define HATCHANNEL 19
// #define PADCHANNEL 14

#define MAXLOWTABLES 5
#define MAXHITABLES 7
#define MAXPADTABLES 18

Switch INSTR1mute(6, true);
Switch INSTR2mute(4, true);
Switch INSTR3mute(2, true);
Switch INSTR4mute(17, true);
Switch INSTR5mute(19, true);
Switch INSTR6mute(24, true);
Switch *INSTRmute[MAXINSTRUMENTS] = {&INSTR1mute, &INSTR2mute, &INSTR3mute, &INSTR4mute, &INSTR5mute, &INSTR6mute};

Counter INSTR1patternPointer(MAXLOWTABLES - 1);
Counter INSTR2patternPointer(MAXHITABLES - 1);
Counter INSTR3patternPointer(MAXHITABLES - 1);
Counter INSTR4patternPointer(MAXHITABLES - 1);
Counter INSTR5patternPointer(MAXHITABLES - 1);
Counter INSTR6patternPointer(MAXPADTABLES - 1);
Counter *INSTRpatternPointer[MAXINSTRUMENTS] = {&INSTR1patternPointer, &INSTR2patternPointer, &INSTR3patternPointer, &INSTR4patternPointer, &INSTR5patternPointer, &INSTR6patternPointer};

//fill with max available samples on pi
Counter INSTR1samplePointer(5);
Counter INSTR2samplePointer(5);
Counter INSTR3samplePointer(5);
Counter INSTR4samplePointer(5);
Counter INSTR5samplePointer(5);
Counter INSTR6samplePointer(5);
Counter *INSTRsamplePointer[MAXINSTRUMENTS] = {&INSTR1samplePointer, &INSTR2samplePointer, &INSTR3samplePointer, &INSTR4samplePointer, &INSTR5samplePointer, &INSTR6samplePointer};

uint8_t encPinA[MAXENCODERS] = {12, 9, 48, 30, 42, 24, 36, 15};
uint8_t encPinB[MAXENCODERS] = {13, 10, 50, 32, 44, 26, 38, 14};
int oldReadA[MAXENCODERS];
int oldReadB[MAXENCODERS];
int readA[MAXENCODERS];
int readB[MAXENCODERS];
int16_t newencoderValue[MAXENCODERS];
int16_t oldencoderValue[MAXENCODERS];

boolean playSteps = true;

boolean lowTable[MAXLOWTABLES][32] = {
    //, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _},
    //, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //..
    {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0}, //4x4
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //tum tum...
    {1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //
    {0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0}  //
};
boolean hiTable[MAXHITABLES][32] = {
    //, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _, |, _, _, _, _, _, _, _},
    //, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //..
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}, //tss
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0}, //tah
    {1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0}, //ti di di
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0}, //tei tei tei
    {1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0}, //ti di di rápido
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}  //tststststststst
};

boolean padTable[MAXPADTABLES][32] = {
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

void ISRtriggerIn()
{
  //playSteps = !playSteps;
}

void setup()
{
  triggerIn.attachCallOnRising(ISRtriggerIn);
  MIDI.begin();

  // Serial.begin(9600);

  // INSTRpatternPointer[0]->setValue(0);
  // INSTRpatternPointer[1]->setValue(0);
  // INSTRpatternPointer[2]->setValue(0);

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
}

void loop()
{
  //read all encoders buttons
  for (uint8_t i = 0; i < MAXENCODERS; i++)
    encoderButtons[i]->readPin();

  //read instrument mute buttons
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    INSTRmute[i]->readPin();

  readEncoder();

  //play mode on and interval ended
  if (playSteps && stepInterval.debounced())
  {
    stepInterval.debounce();                  //start new interval
    uint8_t thisStep = stepCounter.advance(); //advance step pointer
    uint8_t noteVelocity = 0;                 //calculate noteVelocity
    for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    {
      //if not muted and  this instrument step is TRUE
      if ((i <= 0) && (INSTRmute[i]->active() && (lowTable[INSTRpatternPointer[i]->getValue()][thisStep] == 1)))
      {
        noteVelocity = noteVelocity + powint(2, 5 - i);
        INSTRtriggers[i]->start();
      }
      else if ((i <= 4) && (INSTRmute[i]->active() && (hiTable[INSTRpatternPointer[i]->getValue()][thisStep] == 1)))
      {
        noteVelocity = noteVelocity + powint(2, 5 - i);
        INSTRtriggers[i]->start();
      }
      else if ((i == 5) && (INSTRmute[i]->active() && (padTable[INSTRpatternPointer[i]->getValue()][thisStep] == 1)))
      {
        noteVelocity = noteVelocity + powint(2, 5 - i);
        INSTRtriggers[i]->start();
      }
    }
    if (noteVelocity != 0)
      MIDI.sendNoteOn(1, noteVelocity, MIDICHANNEL);
    //    Serial.println(noteVelocity);
  }
  //close triggers
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    INSTRtriggers[i]->compute();
}

void readEncoder()
{
  //only one encoder at time
  uint8_t queuedEncoder = encoderQueue.advance();

  int8_t newRead = 0;
  newRead = compare4EncoderStates(queuedEncoder);
  //if there was any change
  if (newRead != 0)
  {
    newencoderValue[queuedEncoder] += newRead;
    if (oldencoderValue[queuedEncoder] != newencoderValue[queuedEncoder]) //avoid repeat direction change repeat like:  9,10,11,11,10,9,...
    {
      oldencoderValue[queuedEncoder] = newencoderValue[queuedEncoder];
      if ((newencoderValue[queuedEncoder] % 4) == 0)
      {
        encoderChanged(queuedEncoder, newencoderValue[queuedEncoder] / 4, newRead);
      }
    }
  }
}

void encoderChanged(uint8_t _encoder, uint8_t _newValue, int8_t _change)
{
  switch (_encoder)
  {
  case MOODENCODER:
    MIDI.sendNoteOn(MOODMIDINOTE, _newValue, MIDICHANNEL);
    break;

  case MORPHENCODER:
    MIDI.sendNoteOn(MORPHMIDINOTE, _newValue, MIDICHANNEL);
    break;
  default:
    if (!encoderButtons[_encoder]->active()) //change instrument
    {
      if (_change == -1)
      {
        INSTRsamplePointer[_encoder - 2]->reward();
      }
      else
      {
        INSTRsamplePointer[_encoder - 2]->advance();
      }
      MIDI.sendNoteOn(INSTRmidiNote[_encoder - 2], _newValue, MIDICHANNEL);
    }
    else //change step pattern
    {
      if (_change == -1)
      {
        INSTRpatternPointer[_encoder - 2]->reward();
      }
      else
      {
        INSTRpatternPointer[_encoder - 2]->advance();
      }
    }
    break;
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

// //trigger out pins
// Trigger INSTR1trigger(5);
// Trigger INSTR2trigger(3);
// Trigger INSTR3trigger(16);
// Trigger INSTR4trigger(18);
// Trigger INSTR5trigger(23);
// Trigger INSTR6trigger(25);
// Trigger *INSTRtriggers[MAXINSTRUMENTS] = {&INSTR1trigger, &INSTR2trigger, &INSTR3trigger, &INSTR4trigger, &INSTR5trigger, &INSTR6trigger};

// Switch encoderButtonChannel1(52, true);
// Switch encoderButtonChannel2(34, true);
// Switch encoderButtonChannel3(46, true);
// Switch encoderButtonChannel4(28, true);
// Switch encoderButtonChannel5(40, true);
// Switch encoderButtonChannel6(42, true);

// Switch INSTR1mute(6, true);
// Switch INSTR2mute(4, true);
// Switch INSTR3mute(2, true);
// Switch INSTR4mute(17, true);
// Switch INSTR5mute(19, true);
// Switch INSTR6mute(24, true);
