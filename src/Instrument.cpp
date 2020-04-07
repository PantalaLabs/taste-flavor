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
  instrumentIdentifyer = _instr;
  maxPatterns = _maxPatterns;
  solo = false;

  patternIndex = new Counter(_maxPatterns);
  //add euclidean patterns to each instrument pattern collection
  patternIndex = new Counter(_maxPatterns + G_MAXEUCLIDIANPATTERNS);
  for (byte pat = 0; pat < G_MAXEUCLIDIANPATTERNS; pat++)
    for (byte block = 0; block < G_MAXBLOCKS; block++)
      blocks[instrumentIdentifyer][_maxPatterns + 1 + pat][block] = euclidpatterntable[pat][block];

  patternIndex->setInit(1);
  patternIndex->setCyclable(false);
  patternIndex->reset();

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
//return if it is a valid/non valid _step from the actual pattern
bool Instrument::getStep(uint16_t _step)
{
  byte thisBlockByte = blocks[instrumentIdentifyer][patternIndex->getValue()][_step / 8];
  return bitRead(thisBlockByte, 7 - (_step % 8));
}

//return if it is a valid/non valid _step from the selected _pat
bool Instrument::getStep(uint16_t _pat, uint16_t _step)
{
  byte thisBlockByte = blocks[instrumentIdentifyer][_pat][_step / 8];
  return bitRead(thisBlockByte, 7 - (_step % 8));
}

//return if it is a valid/non valid _step from the selected _pat from RAM or ROM sources
bool Instrument::getStep(uint16_t _pat, uint16_t _step, bool _src)
{
  //if search for the original pattern and this pattern was already customized , return the step on the BKP area
  byte thisBlockByte1 = blocks[instrumentIdentifyer][bkpPattern][_step / 8];
  byte thisBlockByte2 = blocks[instrumentIdentifyer][_pat][_step / 8];
  return (bitRead(thisBlockByte1, 7 - (_step % 8)) || bitRead(thisBlockByte2, 7 - (_step % 8)));
}

//reset actual parttern to its default and move pattern index forward
void Instrument::setNextInternalPattern()
{
  resetCustomPatternToOriginal();
  patternIndex->advance();
}

//reset actual parttern to its default and move pattern index reward
void Instrument::setPreviousInternalPattern()
{
  resetCustomPatternToOriginal();
  patternIndex->reward();
}

//set step value 1/0
void Instrument::setStep(uint16_t _step, uint16_t _val)
{
  byte thisBlockByte = blocks[instrumentIdentifyer][patternIndex->getValue()][_step / 8]; //recover the active block for this step
  bitWrite(thisBlockByte, 7 - (_step % 8), _val);                                         //set inverting the significant step
  blocks[instrumentIdentifyer][patternIndex->getValue()][_step / 8] = thisBlockByte;      //save block
}

//change gate lenght size from this instrument
void Instrument::changeGateLenghSize(int8_t _change)
{
  gateLenghtSize += (_change == 1) ? 1 : -1;
  gateLenghtSize = constrain(gateLenghtSize, 0, G_MAXGATELENGHTS);
}

//erase all instrument pattern
void Instrument::eraseInstrumentPattern()
{
  //check if it is customizable
  customizeThisPattern();
  //clear actual pattern
  for (int8_t block = 0; block < G_MAXBLOCKS; block++)
    blocks[instrumentIdentifyer][patternIndex->getValue()][block] = 0;
}

//flags that this pattern will be customized
void Instrument::customizeThisPattern()
{
  //if this pattern isn´t customized
  if (customPattern == 0)
  {
    copyRefPatternToRefPattern(patternIndex->getValue(), bkpPattern); //bkp it to safe area
    customPattern = patternIndex->getValue();                         //set asked pattern as BKPd
  }
  //restore it to original state before make any other customization ???
  // copyRefPatternToRefPattern(_instr, bkpPattern, customizedPattern[_instr]);
}

//copy patterns
void Instrument::copyRefPatternToRefPattern(uint16_t _source, uint16_t _target)
{
  for (uint8_t block = 0; block < G_MAXBLOCKS; block++)
    blocks[instrumentIdentifyer][_target][block] = blocks[instrumentIdentifyer][_source][block];
}

//go back this pattern to its original value
void Instrument::resetCustomPatternToOriginal()
{
  if (!customPattern) //if any pattern was customized , restore it from bkp before do anything
  {
    copyRefPatternToRefPattern(bkpPattern, customPattern); //restore BKPd pattern to its original place
    customPattern = 0;
  }
}

//clear all undo entries
void Instrument::clearUndoArray()
{
  for (uint16_t i = 0; i < maxUndos; i++)
    undoStack[i] = -1;
}

//add one undo operation
void Instrument::addUndoStep(uint16_t _step)
{
  for (uint16_t i = (maxUndos - 1); i > 0; i--)
    undoStack[i] = undoStack[i - 1]; //move all values ahead to open space on stack
  undoStack[0] = _step;
}

//rollback one undo operation
void Instrument::rollbackUndoStep()
{
  setStep(undoStack[0], 0); //remove tapped step
  for (uint16_t i = 0; i < (maxUndos - 1); i++)
    undoStack[i] = undoStack[i + 1]; //move all values ahead to open space on stack
  undoStack[(maxUndos - 1)] = -1;
}

//if there is any rollback operation available
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
bool Instrument::playThisStep(uint16_t _step)
{
  return (!permanentMute && !tappedStep && getStep(_step));
}

//if this is a valid step
bool Instrument::playThisTrigger(uint16_t _step)
{
  return (getStep(_step));
}

//if this pattern is custom and there is an undo available
bool Instrument::undoLastTappedStep()
{
  if (customPattern && undoAvailable())
  {
    rollbackUndoStep();
    return true;
  }
  return false;
}

//set one byte position inside a byte blobk
// byte Instrument::setBytePositionIntoByteBlock(byte byteBlock, byte _stepId)
// {
//   byte position = _stepId / 8;
//   return bitSet(byteBlock, position % 8);
// }

//legacy : convert old bool arrays to new byte block arrays
// void Instrument::legacyboolToByte()
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
// //legacy : convert old euclidean bool arrays to new byte block arrays
// void Instrument::legacyEuclidboolToByte()
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
//         if (boolEuclid[pat][block * 8 + 7 - step])
//           accum = accum | (1 << step);
//       Serial.print(accum);
//       Serial.print(",");
//     }
//     Serial.println("};");
//   }
//   Serial.println("};");
// }
