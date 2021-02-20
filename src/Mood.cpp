/*mood
  Creative Commons License CC-BY-SA
  mood Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "tf_Defines.h"
#include <Counter.h>
#include "Mood.h"

Mood::Mood(uint16_t _maxMoods)
{
  DEBL("Mood started.");

  moodIndex = new Counter(_maxMoods - 1);
  moodIndex->setCyclable(false);

  patterns[0] = new Pattern(0);
  patterns[1] = new Pattern(1);
  patterns[2] = new Pattern(2);
  patterns[3] = new Pattern(3);
  patterns[4] = new Pattern(4);
  patterns[5] = new Pattern(5);

  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    samples[i] = new Counter((_maxMoods * G_MAXINSTRUMENTS) - 1);
    samples[i]->setCyclable(false);
    samples[i]->reset();
  }
  DEBL("Mood created.");
}

//PUBLIC----------------------------------------------------------------------------------------------
void Mood::cue(uint8_t moodId, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5, uint8_t s6)
{
  reset();
  moodIndex->setValue(moodId);
  patterns[0]->patternIndex->setValue(p1);
  patterns[1]->patternIndex->setValue(p2);
  patterns[2]->patternIndex->setValue(p3);
  patterns[3]->patternIndex->setValue(p4);
  patterns[4]->patternIndex->setValue(p5);
  patterns[5]->patternIndex->setValue(p6);
  samples[0]->setValue(s1);
  samples[1]->setValue(s2);
  samples[2]->setValue(s3);
  samples[3]->setValue(s4);
  samples[4]->setValue(s5);
  samples[5]->setValue(s6);
}

void Mood::cueXfadedInstrument(uint16_t _instr, uint16_t _newSampleId, uint16_t _newPatternId, bool _newMute, uint8_t _newLenght)
{
  samples[_instr]->setValue(_newSampleId);
  patterns[_instr]->patternIndex->setValue(_newPatternId);
  patterns[_instr]->muted = _newMute;
  patterns[_instr]->gateLenghtSize = _newLenght;
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
    patterns[i]->bkpMute = false;
    patterns[i]->muted = false;
    samples[i]->reset();
    patterns[i]->patternIndex->reset();
  }
}

void Mood::discardNotXfadedInstrument(uint8_t _instr)
{
  samples[_instr]->setValue(_instr); //point to emty sample value 0 to 5
  patterns[_instr]->patternIndex->reset();
  patterns[_instr]->muted = true;
  patterns[_instr]->gateLenghtSize = 0;
}

void Mood::resetAllCustomPatternsToOriginal()
{
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    patterns[i]->resetCustomPatternToOriginal();
}

void Mood::resetAllGateLenght()
{
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    patterns[i]->gateLenghtSize = 0;
}

void Mood::setSoloInstrument(uint8_t instr)
{
  //before solo or unsolo any specific instrument
  //must check if there is any previous different solo´d instrument
  //to roll back all patterns to its initial mute value
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    //if instrument isnt the chosen one and it is solo´d
    if ((i != instr) && patterns[i]->solo)
    {
      //resto all initial mute condition
      for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
        patterns[i]->muted = patterns[i]->bkpMute;
      //and unsolo it
      patterns[i]->solo = false;
    }

  //now we are able to solo an instrument
  //change instrument solo status
  patterns[instr]->solo = !patterns[instr]->solo;

  //if solo an instrument
  if (patterns[instr]->solo)
  {
    //for all patterns
    for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    {
      //bkp the original mute condition
      patterns[i]->bkpMute = patterns[i]->muted;
      //mute it
      patterns[i]->muted = true;
    }
    //unmute only the chosen instrument
    patterns[instr]->muted = false;
  }
  else
  {
    //if UNsolo an instrument, restore, just restore all initial mute condition
    for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
      patterns[i]->muted = patterns[i]->bkpMute;
  }
}

void Mood::setMuteAllInstruments(bool state)
{
  //this feature is nice to release the break into music.
  //so when mute all

  //to mute all  , we must unsolo any instrument
  //restore any soloed instrument to its initial condition
  //and them permanent mute all patterns
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    patterns[i]->muted = state;
    patterns[i]->bkpMute = false;
    patterns[i]->solo = false;
  }
}