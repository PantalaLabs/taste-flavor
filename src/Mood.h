/*Mood

  Creative Commons License CC-BY-SA
  Mood Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/
#ifndef Mood_h
#define Mood_h

#include "tf_Defines.h"
#include <DebugDefines.h>
#include <Arduino.h>
#include <PantalaDefines.h>
#include <Counter.h>
#include "Pattern.h"

class Mood
{
public:
  Mood(uint16_t _maxMoods);
  Counter *moodIndex;
  String name;
  Pattern *patterns[G_MAXGATELENGHTS];
  Counter *samples[G_MAXGATELENGHTS];
  void cue(uint8_t moodId, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5, uint8_t s6);
  void cueXfadedInstrument(uint16_t _instr, uint16_t _newSampleId, uint16_t _newPatternId, bool _newMute, uint8_t _newLenght);
  void discardNotXfadedInstrument(uint8_t _instr);
  void changeMaxMoods(uint16_t _max);
  void reset();
  void setSoloInstrument(uint8_t instr);
  void setMuteAllInstruments(bool state);

private:
  void resetAllGateLenght();
  void resetAllCustomPatternsToOriginal();
};

#endif
