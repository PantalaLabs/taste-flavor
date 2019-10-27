/*Deck
  Creative Commons License CC-BY-SA
  Deck Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "Deck.h"

Deck::Deck()
{
  deckSamples[0] = new Counter(MAXINSTR1SAMPLES - 1);
  deckSamples[1] = new Counter(MAXINSTR2SAMPLES - 1);
  deckSamples[2] = new Counter(MAXINSTR3SAMPLES - 1);
  deckSamples[3] = new Counter(MAXINSTR4SAMPLES - 1);
  deckSamples[4] = new Counter(MAXINSTR5SAMPLES - 1);
  deckSamples[5] = new Counter(MAXINSTR6SAMPLES - 1);

  deckPatterns[0] = new Counter(MAXINSTR1PATTERNS);
  deckPatterns[1] = new Counter(MAXINSTR2PATTERNS);
  deckPatterns[2] = new Counter(MAXINSTR3PATTERNS);
  deckPatterns[3] = new Counter(MAXINSTR4PATTERNS);
  deckPatterns[4] = new Counter(MAXINSTR5PATTERNS);
  deckPatterns[5] = new Counter(MAXINSTR6PATTERNS);

  patterns = new Patterns();
  Deck::init();
}

//PUBLIC----------------------------------------------------------------------------------------------
void Deck::cue(uint8_t moodId, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6)
{
  patterns->resetAllCustomizedPatternsToOriginal();
  resetAllPermanentMute();
  resetAllGateLenght();
  deckSamples[0]->setValue(moodId * 6);
  deckSamples[1]->setValue(moodId * 6 + 1);
  deckSamples[2]->setValue(moodId * 6 + 2);
  deckSamples[3]->setValue(moodId * 6 + 3);
  deckSamples[4]->setValue(moodId * 6 + 4);
  deckSamples[5]->setValue(moodId * 6 + 5);
  deckPatterns[0]->setValue(p1);
  deckPatterns[1]->setValue(p2);
  deckPatterns[2]->setValue(p3);
  deckPatterns[3]->setValue(p4);
  deckPatterns[4]->setValue(p5);
  deckPatterns[5]->setValue(p6);
}

void Deck::changeGateLenghSize(uint8_t _instrum, int8_t _change)
{
  gateLenghtSize[_instrum] += (_change == 1) ? 1 : -1;
  gateLenghtSize[_instrum] = constrain(gateLenghtSize[_instrum], 0, MAXGATELENGHTS);
}

//PRIVATE----------------------------------------------------------------------------------------------
void Deck::init()
{
  for (uint8_t pat = 0; pat < MAXINSTRUMENTS; pat++) // search for any BKPd pattern
  {
    deckSamples[pat]->setCyclable(false);
    deckPatterns[pat]->setCyclable(false);
    deckPatterns[pat]->setInit(1);
  }
}

void Deck::resetAllGateLenght()
{
  for (uint8_t pat = 0; pat < MAXINSTRUMENTS; pat++)
    gateLenghtSize[pat] = 0; //clear all gate lengh sizes
}

void Deck::resetAllPermanentMute()
{
  for (uint8_t pat = 0; pat < MAXINSTRUMENTS; pat++)
    permanentMute[pat] = 0;
}
