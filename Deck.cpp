/*Deck
  Creative Commons License CC-BY-SA
  Deck Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "Deck.h"

Deck::Deck()
{
  // deckSamples[0] = new Counter(MAXINSTR1SAMPLES - 1);
  // deckSample2 = new Counter(MAXINSTR2SAMPLES - 1);
  // deckSample3 = new Counter(MAXINSTR3SAMPLES - 1);
  // deckSample4 = new Counter(MAXINSTR4SAMPLES - 1);
  // deckSample5 = new Counter(MAXINSTR5SAMPLES - 1);
  // deckSample6 = new Counter(MAXINSTR6SAMPLES - 1);
  // deckPattern1 = new Counter(MAXINSTR1PATTERNS);
  // deckPattern2 = new Counter(MAXINSTR2PATTERNS);
  // deckPattern3 = new Counter(MAXINSTR3PATTERNS);
  // deckPattern4 = new Counter(MAXINSTR4PATTERNS);
  // deckPattern5 = new Counter(MAXINSTR5PATTERNS);
  // deckPattern6 = new Counter(MAXINSTR6PATTERNS);

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

void Deck::init()
{
  for (uint8_t pat = 0; pat < MAXINSTRUMENTS; pat++) // search for any BKPd pattern
  {
    deckSamples[pat]->setCyclable(false);
    deckPatterns[pat]->setCyclable(false);
    deckPatterns[pat]->setInit(1);
  }

  // deckSample1->setCyclable(false);
  // deckSample2->setCyclable(false);
  // deckSample3->setCyclable(false);
  // deckSample4->setCyclable(false);
  // deckSample5->setCyclable(false);
  // deckSample6->setCyclable(false);

  // deckPattern1->setInit(1);
  // deckPattern2->setInit(1);
  // deckPattern3->setInit(1);
  // deckPattern4->setInit(1);
  // deckPattern5->setInit(1);
  // deckPattern6->setInit(1);

  // deckPattern1->setCyclable(false);
  // deckPattern2->setCyclable(false);
  // deckPattern3->setCyclable(false);
  // deckPattern4->setCyclable(false);
  // deckPattern5->setCyclable(false);
  // deckPattern6->setCyclable(false);
}

void Deck::cue(uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5, uint8_t s6, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6)
{
  resetCustomizedPatterns();
  resetAllPermanentMute();
  resetAllGateLenght();
  restoreCustomizedPatternsToOriginalRef();
  // deckSample1->setValue(s1);
  // deckSample2->setValue(s2);
  // deckSample3->setValue(s3);
  // deckSample4->setValue(s4);
  // deckSample5->setValue(s5);
  // deckSample6->setValue(s6);
  // deckPattern1->setValue(p1);
  // deckPattern2->setValue(p2);
  // deckPattern3->setValue(p3);
  // deckPattern4->setValue(p4);
  // deckPattern5->setValue(p5);
  // deckPattern6->setValue(p6);
  // copyRefPatternToPlayingPatterns(0, deckPattern1->getValue());
  // copyRefPatternToPlayingPatterns(1, deckPattern2->getValue());
  // copyRefPatternToPlayingPatterns(2, deckPattern3->getValue());
  // copyRefPatternToPlayingPatterns(3, deckPattern4->getValue());
  // copyRefPatternToPlayingPatterns(4, deckPattern5->getValue());
  // copyRefPatternToPlayingPatterns(5, deckPattern6->getValue());

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
  for (uint8_t pat = 0; pat < MAXINSTRUMENTS; pat++) // search for any BKPd pattern
    copyRefPatternToPlayingPatterns(pat, deckPatterns[pat]->getValue());
}

void Deck::resetCustomizedPatterns()
{
  for (uint8_t pat = 0; pat < MAXINSTRUMENTS; pat++) // search for any BKPd pattern
  {
    if (customizedPattern[pat] != 0) //if any pattern was customized , restore it from bkp before do anything
    {
      copyRefPatternToRefPattern(pat, BKP_PATTERN, customizedPattern[pat]); //restore BKPd pattern to its original place
      customizedPattern[pat] = 0;
    }
  }
}

void Deck::copyRefPatternToRefPattern(uint8_t _instr, uint8_t _source, uint8_t _target)
{
    boolean **thisPattern;
  thisPattern = &refPatterns[_instr];
  for (uint8_t i = 0; i < MAXSTEPS; i++)
    thisPattern[_target][i] = thisPattern[_source][i];

  // for (uint8_t i = 0; i < MAXSTEPS; i++)
  // {
  //   if (_instr == 0)
  //     refPattern1[_target][i] = refPattern1[_source][i];
  //   else if (_instr == 1)
  //     refPattern2[_target][i] = refPattern2[_source][i];
  //   else if (_instr == 2)
  //     refPattern3[_target][i] = refPattern3[_source][i];
  //   else if (_instr == 3)
  //     refPattern4[_target][i] = refPattern4[_source][i];
  //   else if (_instr == 4)
  //     refPattern5[_target][i] = refPattern5[_source][i];
  //   else if (_instr == 5)
  //     refPattern6[_target][i] = refPattern6[_source][i];
  // }
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

void Deck::restoreCustomizedPatternsToOriginalRef()
{
  for (uint8_t pat = 0; pat < MAXINSTRUMENTS; pat++) // search for any BKPd pattern
  {
    if (customizedPattern[pat] != 0) //if any pattern was customized , restore it from bkp before do anything
    {
      copyRefPatternToRefPattern(pat, BKP_PATTERN, customizedPattern[pat]); //restore BKPd pattern to its original place
      customizedPattern[pat] = 0;
    }
  }
}
void Deck::eraseInstrumentPattern(uint8_t _instr)
{
  boolean **thisPattern;
  thisPattern = &refPatterns[_instr];
  for (int8_t step = 0; step < MAXSTEPS; step++)
    thisPattern[_instr][step] = 0;
    // playingPatterns[_instr][step] = 0;
}

// void Deck::copyRefPatternToPlayingPatterns(uint8_t _instr, uint8_t _romPatternTablePointer)
// {
  // // //code repeat protection
  // if ((lastCopied_instr == _instr) && (lastCopied_PatternTablePointer == _romPatternTablePointer))
  //   return;
  // lastCopied_instr = _instr;
  // lastCopied_PatternTablePointer = _romPatternTablePointer;
  // for (uint8_t i = 0; i < MAXSTEPS; i++)
  // {
  //   boolean **thisPattern;
  //   thisPattern = &refPatterns[0];
  //   playingPatterns[_instr][i] = thisPattern[_romPatternTablePointer][i];
  // }

  // for (uint8_t i = 0; i < MAXSTEPS; i++)
  // {
  //   if (_instr == 0)
  //     playingPatterns[_instr][i] = refPattern1[_romPatternTablePointer][i];
  //   else if (_instr == 1)
  //     playingPatterns[_instr][i] = refPattern2[_romPatternTablePointer][i];
  //   else if (_instr == 2)
  //     playingPatterns[_instr][i] = refPattern3[_romPatternTablePointer][i];
  //   else if (_instr == 3)
  //     playingPatterns[_instr][i] = refPattern4[_romPatternTablePointer][i];
  //   else if (_instr == 4)
  //     playingPatterns[_instr][i] = refPattern5[_romPatternTablePointer][i];
  //   else if (_instr == 5)
  //     playingPatterns[_instr][i] = refPattern6[_romPatternTablePointer][i];
  // }
// }

void Deck::setAllPatternAsOriginal()
{
  for (uint8_t i = 0; i < MAXINSTRUMENTS; i++)
    setThisPatternAsOriginal(i);
}

void Deck::setThisPatternAsOriginal(uint8_t _instr)
{
  customizedPattern[_instr] = 0;
}