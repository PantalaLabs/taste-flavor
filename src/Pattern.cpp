/*Pattern
  Creative Commons License CC-BY-SA
  Pattern Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "tf_Defines.h"
#include <Arduino.h>
#include <Counter.h>
#include <PantalaDefines.h>
#include "Pattern.h"

#ifdef DO_SD
#include "SdComm.h"
#endif

Pattern::Pattern(uint8_t _instr, uint8_t _maxPatterns)
{
  instrumentIdentifyer = _instr;
  id = new Counter(_maxPatterns);

#ifdef DO_SD
  sdc = new SdComm(43, true);
  switch (instrumentIdentifyer)
  {
  case 0:
    totalPatterns = G_INTERNALINSTR1PATTERNS;
    sdc->importInstrumentPattern(instrumentIdentifyer, instr1patterntable, G_INTERNALINSTR1PATTERNS);
    break;
  case 1:
    totalPatterns = G_INTERNALINSTR2PATTERNS;
    sdc->importInstrumentPattern(instrumentIdentifyer, instr2patterntable, G_INTERNALINSTR2PATTERNS);
    break;
  case 2:
    totalPatterns = G_INTERNALINSTR3PATTERNS;
    sdc->importInstrumentPattern(instrumentIdentifyer, instr3patterntable, G_INTERNALINSTR3PATTERNS);
    break;
  case 3:
    totalPatterns = G_INTERNALINSTR4PATTERNS;
    sdc->importInstrumentPattern(instrumentIdentifyer, instr4patterntable, G_INTERNALINSTR4PATTERNS);
    break;
  case 4:
    totalPatterns = G_INTERNALINSTR5PATTERNS;
    sdc->importInstrumentPattern(instrumentIdentifyer, instr5patterntable, G_INTERNALINSTR5PATTERNS);
    break;
  case 5:
    totalPatterns = G_INTERNALINSTR6PATTERNS;
    sdc->importInstrumentPattern(instrumentIdentifyer, instr6patterntable, G_INTERNALINSTR6PATTERNS);
    break;
  }
  totalPatterns += sdc->getimportedRecords();
#endif

  for (uint8_t undos = 0; undos < MAXUNDOS; undos++)
    undoStack[undos] = -1;
}

//PUBLIC----------------------------------------------------------------------------------------------
uint8_t Pattern::isThisStepActive(uint8_t _pat, uint8_t _step)
{
  return steps[instrumentIdentifyer][_pat][_step];
}

void Pattern::changeGateLenghSize(int8_t _change)
{
  gateLenghtSize += (_change == 1) ? 1 : -1;
  gateLenghtSize = constrain(gateLenghtSize, 0, G_MAXGATELENGHTS);
}

void Pattern::eraseInstrumentPattern(uint8_t _pat)
{
  //check if it is customizable
  customizeThisPattern(_pat);
  //clear actual pattern
  for (int8_t step = 0; step < G_MAXSTEPS; step++)
    steps[instrumentIdentifyer][_pat][step] = 0;
}

void Pattern::customizeThisPattern(uint8_t _pat)
{
  //if this pattern isn´t customized
  if (customPattern == 0)
  {
    copyRefPatternToRefPattern(_pat, BKPPATTERN); //bkp it to safe area
    customPattern = true;                         //set asked pattern as BKPd
  }
  //restore it to original state before make any other customization ???
  // copyRefPatternToRefPattern(_instr, BKPPATTERN, customizedPattern[_instr]);
}

void Pattern::copyRefPatternToRefPattern(uint16_t _source, uint16_t _target)
{
  for (uint8_t step = 0; step < G_MAXSTEPS; step++)
    steps[instrumentIdentifyer][_target][step] = steps[instrumentIdentifyer][_source][step];
}

void Pattern::resetCustomPatternToOriginal()
{
  if (!customPattern) //if any pattern was customized , restore it from bkp before do anything
  {
    copyRefPatternToRefPattern(BKPPATTERN, customPattern); //restore BKPd pattern to its original place
    customPattern = 0;
  }
}

uint8_t Pattern::getStep(uint8_t _pat, uint8_t _step)
{
  return steps[instrumentIdentifyer][_pat][_step];
}

void Pattern::setStep(uint8_t _pat, uint8_t _step, uint8_t _val)
{
  steps[instrumentIdentifyer][_pat][_step] = (_val == 1) ? 1 : 0;
}

void Pattern::clearUndoArray()
{
  for (uint8_t i = 0; i < MAXUNDOS; i++)
    undoStack[i] = -1;
}

void Pattern::addUndoStep(uint8_t _step)
{
  for (uint8_t i = (MAXUNDOS - 1); i > 0; i--)
    undoStack[i] = undoStack[i - 1]; //move all values ahead to open space on stack
  undoStack[0] = _step;
}

void Pattern::rollbackUndoStep(uint8_t _pat)
{
  setStep(_pat, undoStack[0], 0); //remove tapped step
  for (uint8_t i = 0; i < (MAXUNDOS - 1); i++)
    undoStack[i] = undoStack[i + 1]; //move all values ahead to open space on stack
  undoStack[(MAXUNDOS - 1)] = -1;
}

//if there is any rollback available
uint8_t Pattern::undoAvailable()
{
  return (undoStack[0] != -1);
}

//tap a step
void Pattern::tapStep(uint8_t _pat, uint8_t _step)
{
  customizeThisPattern(_pat); //prepare the new pattern
  setStep(_pat, _step, 1);    //insert new step into Pattern
  addUndoStep(_step);         //insert new step into undo stack
}

//PRIVATE----------------------------------------------------------------------------------------------
