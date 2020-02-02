/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create StepSequences
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "StepSequence.h"
#include <PantalaDefines.h>
#include <Counter.h>

StepSequence::StepSequence(uint8_t _maxSteps, uint8_t _maxParameters)
{
  maxSteps = _maxSteps;
}
void createNewSequence()
{
}

uint16_t StepSequence::getStep(uint16_t _step)
{
  return sequenceArray[_step];
}
uint16_t StepSequence::setStep(uint16_t _step, uint16_t _value)
{
  sequenceArray[_step] = _value;
}
void StepSequence::setParameter(uint8_t _param, uint8_t _index, uint16_t _value)
{
  parameters[_param][_index] = _value;
}
