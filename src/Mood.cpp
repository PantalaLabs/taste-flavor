/*mood
  Creative Commons License CC-BY-SA
  mood Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "tf_Defines.h"
#include "Mood.h"

Mood::Mood(uint16_t _maxMoods)
{
  id = new Counter(_maxMoods - 1);
  id->setCyclable(false);

  patterns[0] = new Pattern(0, G_INTERNALINSTR1PATTERNS);
  patterns[1] = new Pattern(1, G_INTERNALINSTR2PATTERNS);
  patterns[2] = new Pattern(2, G_INTERNALINSTR3PATTERNS);
  patterns[3] = new Pattern(3, G_INTERNALINSTR4PATTERNS);
  patterns[4] = new Pattern(4, G_INTERNALINSTR5PATTERNS);
  patterns[5] = new Pattern(5, G_INTERNALINSTR6PATTERNS);

  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    samples[i] = new Counter((_maxMoods * G_MAXINSTRUMENTS) - 1);
    samples[i]->setCyclable(false);
    samples[i]->reset();
  }
}

//PUBLIC----------------------------------------------------------------------------------------------
void Mood::cue(uint8_t moodId, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6)
{
  reset();
  id->setValue(moodId);
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    samples[i]->setValue((moodId * G_MAXINSTRUMENTS) + i);

  patterns[0]->id->setValue(p1);
  patterns[1]->id->setValue(p2);
  patterns[2]->id->setValue(p3);
  patterns[3]->id->setValue(p4);
  patterns[4]->id->setValue(p5);
  patterns[5]->id->setValue(p6);
}

void Mood::changeMaxMoods(uint16_t _max)
{
  id->setEnd(_max - 1);
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    samples[i]->setEnd((_max * G_MAXINSTRUMENTS) - 1);
}

void Mood::reset()
{
  resetAllCustomPatternsToOriginal();
  resetAllPermanentMute();
  resetAllGateLenght();
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    samples[i]->reset();
    patterns[i]->id->reset();
  }
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

void Mood::resetAllPermanentMute()
{
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    patterns[i]->permanentMute = 0;
}
