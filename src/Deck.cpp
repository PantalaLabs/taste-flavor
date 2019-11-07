/*Deck
  Creative Commons License CC-BY-SA
  Deck Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "tf_Defines.h"
#include "Deck.h"

Deck::Deck(uint16_t _maxMoods, uint16_t maxPat1, uint16_t maxPat2, uint16_t maxPat3, uint16_t maxPat4, uint16_t maxPat5, uint16_t maxPat6)
{
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    deckSamples[i] = new Counter((_maxMoods * G_MAXINSTRUMENTS) - 1);

  deckPatterns[0] = new Counter(maxPat1);
  deckPatterns[1] = new Counter(maxPat2);
  deckPatterns[2] = new Counter(maxPat3);
  deckPatterns[3] = new Counter(maxPat4);
  deckPatterns[4] = new Counter(maxPat5);
  deckPatterns[5] = new Counter(maxPat6);

  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    deckSamples[i]->setCyclable(false);
    deckPatterns[i]->setInit(1);
    deckPatterns[i]->setCyclable(false);
    deckSamples[i]->reset();
    deckPatterns[i]->reset();
  }
  pattern = new Patterns();
}

//PUBLIC----------------------------------------------------------------------------------------------
void Deck::cue(uint8_t moodId, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6)
{
  reset();
  id = moodId;
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    deckSamples[i]->setValue((moodId * G_MAXINSTRUMENTS) + i);

  deckPatterns[0]->setValue(p1);
  deckPatterns[1]->setValue(p2);
  deckPatterns[2]->setValue(p3);
  deckPatterns[3]->setValue(p4);
  deckPatterns[4]->setValue(p5);
  deckPatterns[5]->setValue(p6);
}

void Deck::changeMaxMoods(uint16_t _max)
{
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
    deckSamples[i]->setEnd((_max * G_MAXINSTRUMENTS) - 1);
}

void Deck::reset()
{
  pattern->resetAllCustomizedPatternsToOriginal();
  resetAllPermanentMute();
  resetAllGateLenght();
  for (uint8_t i = 0; i < G_MAXINSTRUMENTS; i++)
  {
    deckSamples[i]->reset();
    deckPatterns[i]->reset();
  }
}
void Deck::changeGateLenghSize(uint8_t _instrum, int8_t _change)
{
  gateLenghtSize[_instrum] += (_change == 1) ? 1 : -1;
  gateLenghtSize[_instrum] = constrain(gateLenghtSize[_instrum], 0, G_MAXGATELENGHTS);
}

//PRIVATE----------------------------------------------------------------------------------------------

void Deck::resetAllGateLenght()
{
  for (uint8_t pat = 0; pat < G_MAXINSTRUMENTS; pat++)
    gateLenghtSize[pat] = 0; //clear all gate lengh sizes
}

void Deck::resetAllPermanentMute()
{
  for (uint8_t pat = 0; pat < G_MAXINSTRUMENTS; pat++)
    permanentMute[pat] = 0;
}
