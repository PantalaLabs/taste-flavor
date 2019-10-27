/*Deck

  Creative Commons License CC-BY-SA
  Deck Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/
#ifndef Deck_h
#define Deck_h

#define MAXSTEPS 64
#define MAXINSTRUMENTS 6
#define MAXINSTR1SAMPLES 3
#define MAXINSTR2SAMPLES 4
#define MAXINSTR3SAMPLES 5
#define MAXINSTR4SAMPLES 5
#define MAXINSTR5SAMPLES 5
#define MAXINSTR6SAMPLES 3
#define MAXINSTR1PATTERNS 2
#define MAXINSTR2PATTERNS 4
#define MAXINSTR3PATTERNS 5
#define MAXINSTR4PATTERNS 3
#define MAXINSTR5PATTERNS 4
#define MAXINSTR6PATTERNS 2
#define BKPPATTERN 0
#define MAXGATELENGHTS 8

#include "Arduino.h"
#include "Counter.h"
#include "PantalaDefines.h"
#include "Patterns.h"

class Deck
{
public:
  Deck();
  Counter *deckSamples[MAXINSTRUMENTS];
  Counter *deckPatterns[MAXINSTRUMENTS];
  Patterns *pattern;
  int8_t gateLenghtSize[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
  byte permanentMute[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
  void cue(uint8_t moodId, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6);
  void changeGateLenghSize(uint8_t _instrum, int8_t _change);

private:
  void init();
  void resetAllPermanentMute();
  void resetAllGateLenght();

};

#endif
