/*mood
  Creative Commons License CC-BY-SA
  mood Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "tf_Defines.h"
#include "Mood.h"

Mood::Mood(uint16_t _maxMoods)
{
#if DO_SERIAL == true
  Serial.print("Mood started.");
#endif

  moodIndex = new Counter(_maxMoods - 1);
  moodIndex->setCyclable(false);

  instruments[0] = new Instrument(0, G_INTERNALINSTR1PATTERNS);
  instruments[1] = new Instrument(1, G_INTERNALINSTR2PATTERNS);
  instruments[2] = new Instrument(2, G_INTERNALINSTR3PATTERNS);
  instruments[3] = new Instrument(3, G_INTERNALINSTR4PATTERNS);
  instruments[4] = new Instrument(4, G_INTERNALINSTR5PATTERNS);
  instruments[5] = new Instrument(5, G_INTERNALINSTR6PATTERNS);

  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    samples[i] = new Counter((_maxMoods * G_MAXINSTRUMENTS) - 1);
    samples[i]->setCyclable(false);
    samples[i]->reset();
  }
#if DO_SERIAL == true
  Serial.println("Mood created.");
#endif
}

//PUBLIC----------------------------------------------------------------------------------------------
void Mood::cue(uint8_t moodId, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6)
{
  reset();
  moodIndex->setValue(moodId);
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    samples[i]->setValue((moodId * G_MAXINSTRUMENTS) + i);
  instruments[0]->patternIndex->setValue(p1);
  instruments[1]->patternIndex->setValue(p2);
  instruments[2]->patternIndex->setValue(p3);
  instruments[3]->patternIndex->setValue(p4);
  instruments[4]->patternIndex->setValue(p5);
  instruments[5]->patternIndex->setValue(p6);
}

void Mood::cueXfadedInstrument(uint16_t _instr, uint16_t _newSampleId, uint16_t _newPatternId, bool _newMute, uint8_t _newLenght)
{
  samples[_instr]->setValue(_newSampleId);
  instruments[_instr]->patternIndex->setValue(_newPatternId);
  instruments[_instr]->permanentMute = _newMute;
  instruments[_instr]->gateLenghtSize = _newLenght;
}

void Mood::changeMaxMoods(uint16_t _max)
{
  moodIndex->setEnd(_max - 1);
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    samples[i]->setEnd((_max * G_MAXINSTRUMENTS) - 1);
}

void Mood::reset()
{
  resetAllCustomPatternsToOriginal();
  resetAllGateLenght();
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    instruments[i]->bkpMute = false;
    instruments[i]->permanentMute = false;
    samples[i]->reset();
    instruments[i]->patternIndex->reset();
  }
}

void Mood::discardNotXfadedInstrument(uint8_t _instr)
{
  samples[_instr]->reset();
  instruments[_instr]->patternIndex->reset();
  instruments[_instr]->permanentMute = true;
  instruments[_instr]->gateLenghtSize = 0;
}

void Mood::resetAllCustomPatternsToOriginal()
{
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    instruments[i]->resetCustomPatternToOriginal();
}

void Mood::resetAllGateLenght()
{
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    instruments[i]->gateLenghtSize = 0;
}

void Mood::setSoloInstrument(uint8_t instr)
{
  //before solo or unsolo any specific instrument
  //must check if there is any previous different solo´d instrument
  //to roll back all instruments to its initial mute value
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    //if instrument isnt the chosen one and it is solo´d
    if ((i != instr) && instruments[i]->solo)
    {
      //resto all initial mute condition
      for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
        instruments[i]->permanentMute = instruments[i]->bkpMute;
      //and unsolo it
      instruments[i]->solo = false;
    }

  //now we are able to solo an instrument
  //change instrument solo status
  instruments[instr]->solo = !instruments[instr]->solo;

  //if must solo an instrument
  if (instruments[instr]->solo)
  {
    //for all instruments
    for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    {
      //bkp the original mute condition
      instruments[i]->bkpMute = instruments[i]->permanentMute;
      //mute it
      instruments[i]->permanentMute = true;
    }
    //unmute only the chosen instrument
    instruments[instr]->permanentMute = false;
  }
  else
  {
    //if must unsolo an instrument, restore, just restore all initial mute condition
    for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
      instruments[i]->permanentMute = instruments[i]->bkpMute;
  }
}