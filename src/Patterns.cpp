/*Patterns
  Creative Commons License CC-BY-SA
  Patterns Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "tf_Defines.h"
#include "Patterns.h"

Patterns::Patterns()
{
  for (uint8_t undos = 0; undos < MAXUNDOS; undos++)
    for (uint8_t instr = 0; instr < G_MAXINSTRUMENTS; instr++)
      undoStack[undos][instr] = -1;
}

//PUBLIC----------------------------------------------------------------------------------------------
uint8_t Patterns::isThisStepActive(uint8_t _instr, uint8_t _pat, uint8_t _step)
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
  for (int8_t step = 0; step < G_MAXSTEPS; step++)
    steps[_instr][_pat][step] = 0;
}

void Patterns::customizeThisPattern(uint8_t _instr, uint8_t _pat)
{
  //if this pattern isn´t customized
  if (customizedPattern[_instr] == 0)
  {
    copyRefPatternToRefPattern(_instr, _pat, BKPPATTERN); //bkp it to safe area
    customizedPattern[_instr] = _pat;                     //set asked pattern as BKPd
  }
  //restore it to original state before make any other customization
  // copyRefPatternToRefPattern(_instr, BKPPATTERN, customizedPattern[_instr]);
}

void Patterns::copyRefPatternToRefPattern(uint8_t _instr, uint8_t _source, uint8_t _target)
{
  for (uint8_t step = 0; step < G_MAXSTEPS; step++)
    steps[_instr][_target][step] = steps[_instr][_source][step];
}

void Patterns::resetAllCustomizedPatternsToOriginal()
{
  for (uint8_t pat = 0; pat < G_MAXINSTRUMENTS; pat++) // search for any BKPd pattern
  {
    if (customizedPattern[pat] != 0) //if any pattern was customized , restore it from bkp before do anything
    {
      copyRefPatternToRefPattern(pat, BKPPATTERN, customizedPattern[pat]); //restore BKPd pattern to its original place
      customizedPattern[pat] = 0;
    }
  }
}

uint8_t Patterns::getStep(uint8_t _instr, uint8_t _pat, uint8_t _step)
{
  return steps[_instr][_pat][_step];
}

void Patterns::setStep(uint8_t _instr, uint8_t _pat, uint8_t _step, uint8_t _val)
{
  steps[_instr][_pat][_step] = (_val == 1) ? 1 : 0;
}

void Patterns::clearUndoArray(uint8_t _instr)
{
  for (uint8_t i = 0; i < MAXUNDOS; i++)
    undoStack[i][_instr] = -1;
}

void Patterns::addUndoStep(uint8_t _instr, uint8_t _step)
{
  for (uint8_t i = (MAXUNDOS - 1); i > 0; i--)
    undoStack[i][_instr] = undoStack[i - 1][_instr]; //move all values ahead to open space on stack
  undoStack[0][_instr] = _step;
}

void Patterns::rollbackUndoStep(uint8_t _instr, uint8_t _pat)
{
  setStep(_instr, _pat, undoStack[0][_instr], 0); //remove tapped step
  for (uint8_t i = 0; i < (MAXUNDOS - 1); i++)
    undoStack[i][_instr] = undoStack[i + 1][_instr]; //move all values ahead to open space on stack
  undoStack[(MAXUNDOS - 1)][_instr] = -1;
}

//if there is any rollback available
uint8_t Patterns::undoAvailable(uint8_t _instr)
{
  return (undoStack[0][_instr] != -1);
}

//tap a step 
void Patterns::tapStep(uint8_t _instr, uint8_t _pat, uint8_t _step)
{
  customizeThisPattern(_instr, _pat); //prepare the new pattern
  setStep(_instr, _pat, _step, 1);    //insert new step into patterns
  addUndoStep(_instr, _step);         //insert new step into undo stack
}

//PRIVATE----------------------------------------------------------------------------------------------
