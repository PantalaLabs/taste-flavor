/*Patterns
  Creative Commons License CC-BY-SA
  Patterns Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "Patterns.h"

Patterns::Patterns()
{
}

//PUBLIC----------------------------------------------------------------------------------------------
byte Patterns::isThisStepActive(uint8_t _instr, uint8_t _pat,  uint8_t _step)
{
  return steps[_instr][_pat][_step];
}

uint8_t Patterns::isThisCustomPattern(uint8_t _instr)
{
  return customizedPattern[_instr];
}

void Patterns::eraseInstrumentPattern(uint8_t _instr, uint8_t _pat)
{
  //check if it is customizable
  customizeThisPattern(_instr, _pat);
  //clear actual pattern
  for (int8_t step = 0; step < MAXSTEPS; step++)
    steps[_instr][_pat][step] = 0;
}

void Patterns::customizeThisPattern(uint8_t _instr, uint8_t _pat)
{
  //if this pattern is already customized
  if (customizedPattern[_instr] != 0)
    //restore it to original state before make any other customization
    copyRefPatternToRefPattern(_instr, BKPPATTERN, customizedPattern[_instr]);
  //now it´s ok to make new custom pattern
  //bkp it to safe area
  copyRefPatternToRefPattern(_instr, _pat, BKPPATTERN);
  //set asked pattern as BKPd
  customizedPattern[_instr] = _pat;
}

void Patterns::copyRefPatternToRefPattern(uint8_t _instr, uint8_t _source, uint8_t _target)
{
  for (uint8_t step = 0; step < MAXSTEPS; step++)
    steps[_instr][_target][step] = steps[_instr][_source][step];
}

void Patterns::resetAllCustomizedPatternsToOriginal()
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

byte Patterns::getStep(uint8_t _instr, uint8_t _pat, uint8_t _step)
{
  return steps[_instr][_pat][_step];
}

void Patterns::setStep(uint8_t _instr, uint8_t _pat, uint8_t _step, byte _val)
{
  steps[_instr][_pat][_step] = (_val == 1) ? 1 : 0;
}

//PRIVATE----------------------------------------------------------------------------------------------
