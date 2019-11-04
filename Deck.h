/*Deck

  Creative Commons License CC-BY-SA
  Deck Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/
#ifndef Deck_h
#define Deck_h

#define MAXSTEPS 64
#define MAXINSTRUMENTS 6
#define BKPPATTERN 0
#define MAXGATELENGHTS 8

#include "Arduino.h"
#include "Counter.h"
#include "PantalaDefines.h"
#include "Patterns.h"

class Deck
{
public:
  Deck(uint16_t _maxMoods, uint16_t maxPat1, uint16_t maxPat2, uint16_t maxPat3, uint16_t maxPat4, uint16_t maxPat5, uint16_t maxPat6);
  int16_t id = -1;
  Counter *deckSamples[MAXINSTRUMENTS];
  Counter *deckPatterns[MAXINSTRUMENTS];
  Patterns *pattern;
  uint8_t gateLenghtSize[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
  uint8_t permanentMute[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
  boolean tappedStep[MAXINSTRUMENTS] = {0, 0, 0, 0, 0, 0};
  void cue(uint8_t moodId, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6);
  void changeGateLenghSize(uint8_t _instrum, int8_t _change);
  void reset();

private:
  void resetAllPermanentMute();
  void resetAllGateLenght();
};

#endif
