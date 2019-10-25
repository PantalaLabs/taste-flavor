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
  Deck::init();
}

//PUBLIC----------------------------------------------------------------------------------------------
void Deck::cue(uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5, uint8_t s6, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6)
{
  resetAllCustomizedPatterns();
  resetAllPermanentMute();
  resetAllGateLenght();
  deckSamples[0]->setValue(s1);
  deckSamples[1]->setValue(s2);
  deckSamples[2]->setValue(s3);
  deckSamples[3]->setValue(s4);
  deckSamples[4]->setValue(s5);
  deckSamples[5]->setValue(s6);
  deckPatterns[0]->setValue(p1);
  deckPatterns[1]->setValue(p2);
  deckPatterns[2]->setValue(p3);
  deckPatterns[3]->setValue(p4);
  deckPatterns[4]->setValue(p5);
  deckPatterns[5]->setValue(p6);
}

byte Deck::isThisStepActive(uint8_t _instr, uint8_t _step)
{
  return refPatterns[_instr][deckPatterns[_instr]->getValue()][_step];
}

uint8_t Deck::isThisCustomPattern(uint8_t _instr)
{
  return customizedPattern[_instr];
}
void Deck::eraseInstrumentPattern(uint8_t _instr, uint8_t _pat)
{
  //check if it is customizable
  customizeInstrumentPattern(_instr, _pat);
  //clear actual pattern
  for (int8_t step = 0; step < MAXSTEPS; step++)
    refPatterns[_instr][_pat][step] = 0;
}

void Deck::changeGateLenghSize(uint8_t _instrum, int8_t _change)
{
  gateLenghtSize[_instrum] += (_change == 1) ? 1 : -1;
  gateLenghtSize[_instrum] = constrain(gateLenghtSize[_instrum], 0, MAXGATELENGHTS);
}

void Deck::customizeInstrumentPattern(uint8_t _instr, uint8_t _pat)
{
  //if this pattern is already customized
  if (customizedPattern[_instr] != 0)
  {
    //restore it to original state before make any other customization
    for (int8_t step = 0; step < MAXSTEPS; step++)
      refPatterns[_instr][customizedPattern[_instr]][step] = refPatterns[_instr][BKPPATTERN][step];
  }
  //now it´s ok to make new custom pattern
  //bkp it to safe area
  for (int8_t step = 0; step < MAXSTEPS; step++)
    refPatterns[_instr][BKPPATTERN][step] = refPatterns[_instr][_pat][step];
  //set asked pattern as BKPd
  customizedPattern[_instr] = _pat;
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

void Deck::resetAllCustomizedPatterns()
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

//========++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Deck::bkpPatternToCustomize(uint8_t _instr, uint8_t _source)
{
  //if this pattern wasnt BKPed yet
  if (customizedPattern[_instr] == 0)
    copyRefPatternToRefPattern(_instr, _source, BKPPATTERN);
}

void Deck::changePattern(uint8_t _instr, uint8_t _val)
{
  deckPatterns[_instr] += _val;
}

void Deck::copyRefPatternToRefPattern(uint8_t _instr, uint8_t _source, uint8_t _target)
{
  for (uint8_t step = 0; step < MAXSTEPS; step++)
    refPatterns[_instr][_target][step] = refPatterns[_instr][_source][step];
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

void Deck::setAllPatternAsOriginal()
{
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    setThisPatternAsOriginal(i, 0);
}

void Deck::setThisPatternAsOriginal(uint8_t _instr, byte _orig)
{
  customizedPattern[_instr] = _orig;
}