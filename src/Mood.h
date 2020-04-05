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
#include "Instrument.h"

class Mood
{
public:
  Mood(uint16_t _maxMoods);
  Counter *id;
  String name;
  Instrument *instruments[G_MAXGATELENGHTS];
  Counter *samples[G_MAXGATELENGHTS];
  void cue(uint8_t moodId, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6);
  void cueXfadedInstrument(uint16_t _instr, uint16_t _newSampleId, uint16_t _newPatternId, boolean _newMute, uint8_t _newLenght);
  void discardNotXfadedInstrument(uint8_t _instr);
  void changeMaxMoods(uint16_t _max);
  void reset();
  int8_t getSoloInstrument();
  void setSoloInstrument(uint8_t instr);

private:
  void resetAllPermanentMute();
  void resetAllGateLenght();
  void resetAllCustomPatternsToOriginal();
};

#endif
