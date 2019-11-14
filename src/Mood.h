/*Mood

  Creative Commons License CC-BY-SA
  Mood Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/
#ifndef Mood_h
#define Mood_h

#include "tf_Defines.h"
#include <Arduino.h>
#include <PantalaDefines.h>
#include "Pattern.h"

class Mood
{
public:
  Mood(uint16_t _maxMoods);
  Counter *id;
  String name;
  Pattern *patterns[G_MAXGATELENGHTS];
  Counter *samples[G_MAXGATELENGHTS];
  void resetAllPermanentMute();
  void resetAllGateLenght();
  void resetAllCustomPatternsToOriginal();

  void cue(uint8_t moodId, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6);
  void changeMaxMoods(uint16_t _max);
  void reset();

private:

};

#endif
