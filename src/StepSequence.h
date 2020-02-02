/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create StepSequences
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/
#ifndef StepSequence_h
#define StepSequence_h

#include <Arduino.h>
#include <PantalaDefines.h>
#include <Counter.h>

class StepSequence
{
public:
  StepSequence(uint8_t _maxSteps, uint8_t _maxParameters);
  void createNewSequence();
  uint16_t getStep(uint16_t _step);
  uint16_t setStep(uint16_t _step, uint16_t _value);
  void setParameter(uint8_t _param, uint8_t _index, uint16_t _value);

private:
  uint8_t maxSteps;
  uint8_t maxParameters;
  int16_t parameters[20][4]; //min , max, final wheighted , multiplier
  uint16_t sequenceArray[128];
};
#endif
