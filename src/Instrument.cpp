/*Instrument
  Creative Commons License CC-BY-SA
  Instrument Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "tf_Defines.h"
#include <Arduino.h>
#include <Counter.h>
#include <PantalaDefines.h>
#include "Instrument.h"

#if DO_SD == true
#include "SdComm.h"
#endif

Instrument::Instrument(uint16_t _instr, uint16_t _maxPatterns)
{
#if DO_SERIAL == true
  Serial.print("Instrument started.");
#endif
  solo = false;
  instrumentIdentifyer = _instr;
  maxPatterns = _maxPatterns;

  id = new Counter(_maxPatterns);
  //add euclidean patterns to each pattern step
  id = new Counter(_maxPatterns + G_MAXEUCLIDIANPATTERNS);
  for (byte pat = 0; pat < G_MAXEUCLIDIANPATTERNS; pat++)
    for (byte block = 0; block < G_MAXBLOCKS; block++)
      blocks[instrumentIdentifyer][_maxPatterns + 1 + pat][block] = euclidpatterntable[pat][block];

  id->setInit(1);
  id->setCyclable(false);
  id->reset();

#if DO_SD == true
  sdc = new SdComm(SD_CS, true);
  switch (instrumentIdentifyer)
  {
  case 0:
    totalPatterns = G_INTERNALINSTR1PATTERNS;
    sdc->importInstrumentPattern(instrumentIdentifyer + 1, instr1patterntable, G_INTERNALINSTR1PATTERNS);
    break;
  case 1:
    totalPatterns = G_INTERNALINSTR2PATTERNS;
    sdc->importInstrumentPattern(instrumentIdentifyer + 1, instr2patterntable, G_INTERNALINSTR2PATTERNS);
    break;
  case 2:
    totalPatterns = G_INTERNALINSTR3PATTERNS;
    sdc->importInstrumentPattern(instrumentIdentifyer + 1, instr3patterntable, G_INTERNALINSTR3PATTERNS);
    break;
  case 3:
    totalPatterns = G_INTERNALINSTR4PATTERNS;
    sdc->importInstrumentPattern(instrumentIdentifyer + 1, instr4patterntable, G_INTERNALINSTR4PATTERNS);
    break;
  case 4:
    totalPatterns = G_INTERNALINSTR5PATTERNS;
    sdc->importInstrumentPattern(instrumentIdentifyer + 1, instr5patterntable, G_INTERNALINSTR5PATTERNS);
    break;
  case 5:
    totalPatterns = G_INTERNALINSTR6PATTERNS;
    sdc->importInstrumentPattern(instrumentIdentifyer + 1, instr6patterntable, G_INTERNALINSTR6PATTERNS);
    break;
  }
  totalPatterns += sdc->getimportedRecords();
#endif

  clearUndoArray();
#if DO_SERIAL == true
  Serial.println("Instrument created.");
#endif
}

//PUBLIC----------------------------------------------------------------------------------------------
//return _step from the actual pattern
boolean Instrument::getStep(uint16_t _step)
{
  byte getThisBlock = blocks[instrumentIdentifyer][id->getValue()][_step / 8];
  return bitRead(getThisBlock, 7 - (_step % 8));
}

boolean Instrument::getStep(uint16_t _pat, uint16_t _step)
{
  byte getThisBlock = blocks[instrumentIdentifyer][_pat][_step / 8];
  return bitRead(getThisBlock, 7 - (_step % 8));
}

boolean Instrument::getStep(uint16_t _pat, uint16_t _step, boolean _src)
{
  //if search for the original pattern and this pattern was already customized , return the step on the BKP area
  byte getThisBlock1 = blocks[instrumentIdentifyer][bkpPattern][_step / 8];
  byte getThisBlock2 = blocks[instrumentIdentifyer][_pat][_step / 8];
  return (bitRead(getThisBlock1, 7 - (_step % 8)) || bitRead(getThisBlock2, 7 - (_step % 8)));
}

void Instrument::setNextInternalPattern()
{
  resetCustomPatternToOriginal();
  id->advance();
}

void Instrument::setPreviousInternalPattern()
{
  resetCustomPatternToOriginal();
  id->reward();
}

void Instrument::setStep(uint16_t _step, uint16_t _val)
{
  byte getThisBlock = blocks[instrumentIdentifyer][id->getValue()][_step / 8]; //recover the active block for this step
  bitSet(getThisBlock, 7 - (_step % 8)) = (_val == 1) ? 1 : 0;                 //set inverting the significant step
  blocks[instrumentIdentifyer][id->getValue()][getThisBlock] = getThisBlock;   //save block
}

void Instrument::changeGateLenghSize(int8_t _change)
{
  gateLenghtSize += (_change == 1) ? 1 : -1;
  gateLenghtSize = constrain(gateLenghtSize, 0, G_MAXGATELENGHTS);
}

void Instrument::eraseInstrumentPattern()
{
  //check if it is customizable
  customizeThisPattern();
  //clear actual pattern
  for (int8_t block = 0; block < G_MAXBLOCKS; block++)
    blocks[instrumentIdentifyer][id->getValue()][block] = 0;
}

void Instrument::customizeThisPattern()
{
  //if this pattern isn´t customized
  if (customPattern == 0)
  {
    copyRefPatternToRefPattern(id->getValue(), bkpPattern); //bkp it to safe area
    customPattern = id->getValue();                         //set asked pattern as BKPd
  }
  //restore it to original state before make any other customization ???
  // copyRefPatternToRefPattern(_instr, bkpPattern, customizedPattern[_instr]);
}

void Instrument::copyRefPatternToRefPattern(uint16_t _source, uint16_t _target)
{
  for (uint8_t block = 0; block < G_MAXBLOCKS; block++)
    blocks[instrumentIdentifyer][_target][block] = blocks[instrumentIdentifyer][_source][block];
}

void Instrument::resetCustomPatternToOriginal()
{
  if (!customPattern) //if any pattern was customized , restore it from bkp before do anything
  {
    copyRefPatternToRefPattern(bkpPattern, customPattern); //restore BKPd pattern to its original place
    customPattern = 0;
  }
}

void Instrument::clearUndoArray()
{
  for (uint16_t i = 0; i < maxUndos; i++)
    undoStack[i] = -1;
}

void Instrument::addUndoStep(uint16_t _step)
{
  for (uint16_t i = (maxUndos - 1); i > 0; i--)
    undoStack[i] = undoStack[i - 1]; //move all values ahead to open space on stack
  undoStack[0] = _step;
}

void Instrument::rollbackUndoStep()
{
  setStep(undoStack[0], 0); //remove tapped step
  for (uint16_t i = 0; i < (maxUndos - 1); i++)
    undoStack[i] = undoStack[i + 1]; //move all values ahead to open space on stack
  undoStack[(maxUndos - 1)] = -1;
}

//if there is any rollback available
uint16_t Instrument::undoAvailable()
{
  return (undoStack[0] != -1);
}

//tap a step
void Instrument::tapStep(uint16_t _step)
{
  customizeThisPattern(); //prepare the new pattern
  setStep(_step, 1);      //insert new step into Pattern
  addUndoStep(_step);     //insert new step into undo stack
}

//if this step isnt silenced , neither tapped and a triggered 1 step
boolean Instrument::playThisStep(uint16_t _step)
{
  return (!permanentMute && !tappedStep && getStep(_step));
}

//if this is a valid step
boolean Instrument::playThisTrigger(uint16_t _step)
{
  return (getStep(_step));
}

//if this pattern is custom and there is an undo available
boolean Instrument::undoLastTappedStep()
{
  if (customPattern && undoAvailable())
  {
    rollbackUndoStep();
    return true;
  }
  return false;
}

byte Instrument::setBytePositionIntoByteBlock(byte byteBlock, byte _stepId)
{
  byte position = _stepId / 8;
  return bitSet(byteBlock, position % 8);
}

//legacy : convert old boolean arrays to new byte block arrays
// void Instrument::legacyBooleanToByte()
// {
//   Serial.print("table");
//   Serial.print(instrumentIdentifyer);
//   Serial.println("[][]={");
//   for (byte pat = 0; pat < maxPatterns + 1; pat++)
//   {
//     Serial.print("{");
//     for (byte block = 0; block < 8; block++)
//     {
//       byte accum = 0;
//       for (byte step = 0; step < 8; step++)
//         if (steps[instrumentIdentifyer][pat][block * 8 + 7 - step])
//           accum = accum | (1 << step);
//       Serial.print(accum);
//       Serial.print(",");
//     }
//     Serial.println("};");
//   }
//   Serial.println("};");
// }
// //legacy : convert old euclidean boolean arrays to new byte block arrays
// void Instrument::legacyEuclidBooleanToByte()
// {
//   Serial.print("euclid");
//   Serial.println("[][]={");
//   for (byte pat = 0; pat < G_MAXEUCLIDIANPATTERNS; pat++)
//   {
//     Serial.print("{");
//     for (byte block = 0; block < 8; block++)
//     {
//       byte accum = 0;
//       for (byte step = 0; step < 8; step++)
//         if (booleanEuclid[pat][block * 8 + 7 - step])
//           accum = accum | (1 << step);
//       Serial.print(accum);
//       Serial.print(",");
//     }
//     Serial.println("};");
//   }
//   Serial.println("};");
// }
