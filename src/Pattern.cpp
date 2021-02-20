/*Pattern
  Creative Commons License CC-BY-SA
  Pattern Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/
#include "tf_Defines.h"
#include <DebugDefines.h>
#include <Arduino.h>
#include <Counter.h>
#include <PantalaDefines.h>
#include "Pattern.h"
#include <SPI.h>
#include <SD.h>

Pattern::Pattern(uint8_t _instr)
{
  char readChar;
  String filename = "pattern";
  String token;
  uint8_t importedRecords = 0;
  uint8_t patIdx = 0;

  patternIndex = new Counter(128);
  patternIndex->setInit(1);
  patternIndex->setCyclable(false);
  patternIndex->reset();

  filename += String(_instr);
  filename += ".txt";
  DEB("opening ");
  DEBL(filename);
  patternFile = SD.open(filename, O_READ);
  if (patternFile)
  {
    while (patternFile.available())
    {
      importedRecords++;
      token = "";
      readChar = patternFile.read();
      while ((readChar != '\n') && (readChar != ',') && (patternFile.available()))
      {
        token.concat(readChar);
        readChar = patternFile.read();
      }
      patIdx = token.toInt();
      for (uint8_t step = 0; step < G_MAXSTEPS; step++)
      {
        readChar = patternFile.read();
        token = "";
        token.concat(readChar);
        bool stepValue = token.toInt();
        setStep(patIdx, step, stepValue);
      }
      readChar = patternFile.read();
      readChar = patternFile.read();
    }
    patternFile.close();
    DEBL("file closed.");
  }
  else
  {
    patternFile.close();
    DEB("error while opening ");
    DEBL(filename);
  }
  DEB("imported ");
  DEB(importedRecords);
  DEBL(" patterns");
  patternIndex->setEnd(importedRecords);
  patternIndex->setValue(1);
  clearUndoArray();
  DEBL("Pattern created.");
  delay(10);
}

//points patternindex counter to desired value
void Pattern::setPatternIndex(uint16_t idx)
{
  patternIndex->setValue(idx);
}

//get especific bit from a sourced blobkbyte
bool Pattern::getBlockBit(uint32_t blockByte, uint8_t s)
{
  return bitRead(blockByte, BITPATTERN(s));
}

//get especific bit from a sourced pattern array blockbyte
bool Pattern::getBit(uint8_t pat, uint8_t s)
{
  return getBlockBit(pattern[pat].stepblock[s / G_BLOCKBITS], s);
}

//get full int32 blockbyte from pattern array
uint32_t Pattern::getBlockByte(uint8_t pat, uint8_t s)
{
  return pattern[pat].stepblock[s / G_BLOCKBITS];
}

//change a step bit 1/0 of any blockbit
uint32_t Pattern::setBlockBit(uint8_t p, uint8_t s, bool v)
{
  uint32_t blockByte = getBlockByte(p, s);
  bitWrite(blockByte, BITPATTERN(s), ((v == 0) ? 0 : 1));
  return blockByte;
}

//PUBLIC----------------------------------------------------------------------------------------------

//set step value 1/0 of actual playing pattern
void Pattern::setStep(uint8_t _step, bool _val)
{
  pattern[patternIndex->getValue()].stepblock[_step / G_BLOCKBITS] = setBlockBit(patternIndex->getValue(), _step, _val); //save block
}

//set step value 1/0 of any pattern
void Pattern::setStep(uint8_t _pat, uint8_t _step, bool _val)
{
  pattern[_pat].stepblock[_step / G_BLOCKBITS] = setBlockBit(_pat, _step, _val); //save block
}

//return if it is a valid/non valid _step from the actual pattern
bool Pattern::getStep(uint8_t _step)
{
  return getBit(patternIndex->getValue(), _step);
}

//return if it is a valid/non valid _step from the selected _pat
bool Pattern::getStep(uint8_t _pat, uint8_t _step)
{
  return getBit(_pat, _step);
}

//get entire nBits block
uint32_t Pattern::getBlock(uint8_t _pat, uint8_t _step)
{
  return pattern[_pat].stepblock[_step / G_BLOCKBITS];
}

//reset actual parttern to its default and move pattern index forward
void Pattern::setNextInternalPattern()
{
  resetCustomPatternToOriginal();
  patternIndex->advance();
}

//reset actual parttern to its default and move pattern index reward
void Pattern::setPreviousInternalPattern()
{
  resetCustomPatternToOriginal();
  patternIndex->reward();
}

//change gate lenght size from this instrument
void Pattern::changeGateLenghSize(int8_t _change)
{
  gateLenghtSize += (_change == 1) ? 1 : -1;
  gateLenghtSize = constrain(gateLenghtSize, 0, G_MAXGATELENGHTS);
}

//erase all instrument pattern
void Pattern::erasePattern()
{
  //check if it is customizable
  customizeThisPattern();
  //clear actual pattern
  for (int8_t block = 0; block < (G_MAXSTEPS / G_BLOCKBITS); block++)
    pattern[patternIndex->getValue()].stepblock[block] = 0;
}

//verify if there is al least 1 step true
bool Pattern::emptyPattern()
{
  for (int8_t step = 0; step < G_MAXSTEPS; step++)
    if (getStep(patternIndex->getValue(), step))
      return false;
  return true;
}

//flags that this pattern will be customized
void Pattern::customizeThisPattern()
{
  //if this pattern isn´t customized
  if (customPattern == 0)
  {
    copyRefPatternToRefPattern(patternIndex->getValue(), BKPPATTERN); //bkp it to safe area
    customPattern = patternIndex->getValue();                         //set asked pattern as BKPd
  }
  //restore it to original state before make any other customization ???
  // copyRefPatternToRefPattern(_instr, BKPPATTERN, customizedPattern[_instr]);
}

//copy patterns
void Pattern::copyRefPatternToRefPattern(uint8_t _source, uint8_t _target)
{
  for (uint8_t block = 0; block < (G_MAXSTEPS / G_BLOCKBITS); block++)
    pattern[_target].stepblock[block] = pattern[_source].stepblock[block];
}

//if any pattern was customized , restore it from bkp before do anything
void Pattern::resetCustomPatternToOriginal()
{
  if (!customPattern)
  {
    //copy BKPd pattern to its original place
    copyRefPatternToRefPattern(BKPPATTERN, customPattern);
    //clear bkp pattern area
    //for (uint8_t s = 0;s < G_MAXSTEPS ; s++)
    //  setStep(uint8_t BKPPATTERN, s, 0);
    for (uint8_t block = 0; block < (G_MAXSTEPS / G_BLOCKBITS); block++)
      pattern[0].stepblock[block] = 0;
    customPattern = 0;
  }
}

//clear all undo entries
void Pattern::clearUndoArray()
{
  for (uint16_t i = 0; i < maxUndos; i++)
    undoStack[i] = -1;
}

//add one undo operation
void Pattern::addUndoStep(uint8_t _step)
{
  for (uint16_t i = (maxUndos - 1); i > 0; i--)
    undoStack[i] = undoStack[i - 1]; //move all values ahead to open space on stack
  undoStack[0] = _step;
}

//rollback one undo operation
void Pattern::rollbackUndoStep()
{
  setStep(undoStack[0], 0); //remove tapped step
  for (uint16_t i = 0; i < (maxUndos - 1); i++)
    undoStack[i] = undoStack[i + 1]; //move all values ahead to open space on stack
  undoStack[(maxUndos - 1)] = -1;
}

//if there is any rollback operation available
bool Pattern::undoAvailable()
{
  return (undoStack[0] != -1);
}

//tap a step
void Pattern::tapStep(uint8_t _step)
{
  customizeThisPattern(); //prepare the new pattern
  setStep(_step, 1);      //insert new step into Pattern
  addUndoStep(_step);     //insert new step into undo stack
}

//if this step isnt silenced , neither tapped and a triggered 1 step
bool Pattern::playThisStep(uint8_t _step)
{
  return (!muted && !tappedStep && getStep(_step));
}

//if this is a valid step
bool Pattern::playThisTrigger(uint8_t _step)
{
  return (getStep(_step));
}

//if this pattern is custom and there is an undo available
bool Pattern::undoLastTappedStep()
{
  if (customPattern && undoAvailable())
  {
    rollbackUndoStep();
    return true;
  }
  return false;
}
