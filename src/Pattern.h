/*Pattern

  Creative Commons License CC-BY-SA
  Pattern Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/
#ifndef Pattern_h
#define Pattern_h

#include "tf_Defines.h"
#include <Arduino.h>
#include <Counter.h>
#include <PantalaDefines.h>
#include <SPI.h>
#include <SD.h>

#define BKPPATTERN 0
class Pattern
{
public:
  Pattern(uint8_t _instr);
  Counter *patternIndex;
  uint8_t customPattern = 0;
  int8_t gateLenghtSize = 0;
  bool solo = false;
  bool muted = false;
  bool bkpMute = false;
  bool tappedStep = false;
  void resetCustomPatternToOriginal();
  void changeGateLenghSize(int8_t _change);
  void erasePattern();
  void customizeThisPattern();
  bool getStep(uint8_t _step);
  bool getStep(uint8_t _pat, uint8_t _step);
  void setStep(uint8_t _step, bool _val);
  void setStep(uint8_t _pat, uint8_t _step, bool _val);
  void setPatternIndex(uint16_t idx);
  void setNextInternalPattern();
  void setPreviousInternalPattern();
  bool undoLastTappedStep();
  bool undoAvailable();
  void clearUndoArray();
  void rollbackUndoStep();
  void addUndoStep(uint8_t _step);
  void tapStep(uint8_t _step);
  bool playThisStep(uint8_t _step);
  bool playThisTrigger(uint8_t _step);
  bool emptyPattern();

private:
  File patternFile;
  uint8_t maxUndos = G_MAXSTEPS;
  uint8_t undoStack[G_MAXSTEPS];
  void init();
  uint32_t getBlock(uint8_t _pat, uint8_t _step);
  bool getBlockBit(uint32_t blockByte, uint8_t s);
  bool getBit(uint8_t pat, uint8_t s);
  uint32_t getBlockByte(uint8_t pat, uint8_t s);
  uint32_t setBlockBit(uint8_t p, uint8_t s, bool v);
  void copyRefPatternToRefPattern(uint8_t _source, uint8_t _target);

  typedef struct
  {
    uint32_t stepblock[G_MAXSTEPS / G_BLOCKBITS];
  } record_type;
  record_type pattern[128];

#define BITPATTERN(b) ((G_BLOCKBITS - 1) - (b % G_BLOCKBITS))

  //https://forum.arduino.cc/index.php?topic=642706.0
  //easy acessible by :
  //steps[instrument][pattern][step];
  //uint16_t (*blocks[6])[G_MAXBLOCKS] = {instr1patterntable, instr2patterntable, instr3patterntable, instr4patterntable, instr5patterntable, instr6patterntable};
  //or
  //uint8_t *steps[G_MAXINSTRUMENTS] = {&instr1[0][0], &instr2[0][0], &instr3[0][0], &instr4[0][0], &instr5[0][0], &instr6[0][0]};
  //return steps[_instr][PatternPattern[_instr]->getValue() * G_MAXSTEPS + _step];
  //legacy
  //uint8_t (*steps[6])[G_MAXINSTRUMENTS] = {instr1patterntable, instr2patterntable, instr3patterntable, instr4patterntable, instr5patterntable, instr6patterntable};
};

#endif
