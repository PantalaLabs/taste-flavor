/*Instrument

  Creative Commons License CC-BY-SA
  Instrument Library / taste & Flavor by Gibran Curtiss Salomão/Pantala Labs is licensed
  under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/
#ifndef Instrument_h
#define Instrument_h

#include "tf_Defines.h"
#include <Arduino.h>
#include <Counter.h>
#include <PantalaDefines.h>

#ifdef DO_SD == true
#include "SdComm.h"
#endif

class Instrument
{
public:
  Instrument(uint16_t _instr, uint16_t _maxPatterns);

  Counter *id;

  uint16_t customPattern = 0;
  int8_t gateLenghtSize = 0;
  boolean permanentMute = 0;
  boolean tappedStep = 0;
  byte maxPatterns = 0;

  void resetCustomPatternToOriginal();
  void changeGateLenghSize(int8_t _change);
  void eraseInstrumentPattern();
  void customizeThisPattern();
  boolean getStep(uint16_t _step);
  boolean getStep(uint16_t _pat, uint16_t _step);
  boolean getStep(uint16_t _pat, uint16_t _step, boolean _src);

  void setNextInternalPattern();
  void setPreviousInternalPattern();

  void setStep(uint16_t _step, uint16_t _val);
  void copystepTostep(uint16_t _source, uint16_t _target);
  boolean undoLastTappedStep();
  uint16_t undoAvailable();
  void clearUndoArray();
  void rollbackUndoStep();
  void addUndoStep(uint16_t _step);
  void tapStep(uint16_t _step);
  boolean playThisStep(uint16_t _step);
  boolean playThisTrigger(uint16_t _step);
  byte setBytePositionIntoByteBlock(byte byteBlock, byte _stepId);
  boolean solo;
  
  //legacy
  void legacyBooleanToByte();
  void legacyEuclidBooleanToByte();
  
private:
  uint16_t bkpPattern = 0;
  void copyRefPatternToRefPattern(uint16_t _source, uint16_t _target);
  void init();
  uint16_t maxUndos = G_MAXSTEPS;
  int8_t undoStack[G_MAXSTEPS];
  int8_t instrumentIdentifyer; //to link this Class to an instrument identifyer (1 to 6)
  uint16_t totalPatterns;

#ifdef DO_SD == true
  SdComm *sdc;
#endif

  //position [0] reserved to BKP pattern , position [1] reserved to "MUTE" pattern
  byte instr1patterntable[G_INTERNALINSTR1PATTERNS + 1 + G_MAXEUCLIDIANPATTERNS][G_MAXBLOCKS] = {
      {0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0},
      {136, 136, 136, 136, 136, 136, 136, 136}, //2 10001000
      {136, 168, 136, 168, 136, 168, 136, 168}, //3 10001000 10101000
      {128, 128, 128, 128, 128, 128, 128, 128}, //4 10000000
  };

  byte instr2patterntable[G_INTERNALINSTR2PATTERNS + 1 + G_MAXEUCLIDIANPATTERNS][G_MAXBLOCKS] = {
      {0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0},
      {128, 128, 128, 128, 128, 128, 128, 128}, //2 10000000
      {34, 34, 34, 34, 34, 34, 34, 34},         //3 00100010
      {136, 136, 136, 136, 136, 136, 136, 136}, //4 10001000
      {255, 255, 255, 255, 255, 255, 255, 255}, //5 11111111
      {10, 10, 10, 10, 10, 10, 10, 10},         //6 00001010
      {8, 72, 8, 72, 8, 72, 8, 72},             //7 00001000 01001000
      {170, 170, 170, 170, 170, 170, 170, 170}, //8 10101010
      {42, 42, 42, 42, 42, 42, 42, 42},         //9 00101010
      {146, 75, 36, 146, 146, 75, 36, 150},     //10 10010010 01001011 00100100 10010010 10010010 01001011 00100100 10010110
      {82, 82, 82, 82, 82, 82, 82, 82},         //11 01010010 01010010 01010010 01010010 01010010 01010010 01010010 01010010
      {80, 0, 80, 0, 80, 0, 80, 0},             //12 01010000 00000000
      {82, 0, 82, 0, 82, 0, 82, 0},             //13 01010010 00000000
      {18, 16, 18, 18, 18, 16, 18, 18},         //14 00010010 00010000 00010010 00010010 00010010 00010000 00010010 00010010

  };

  byte instr3patterntable[G_INTERNALINSTR3PATTERNS + 1 + G_MAXEUCLIDIANPATTERNS][G_MAXBLOCKS] = {
      {0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0},
      {136, 168, 136, 168, 136, 168, 136, 168}, //2 10001000 10101000
      {8, 8, 8, 8, 8, 8, 8, 8},                 //3 00001000
      {162, 162, 162, 162, 162, 162, 162, 162}, //4 10100010
      {34, 34, 34, 34, 34, 34, 34, 34},         //5 00100010
      {65, 65, 65, 65, 65, 65, 65, 65},         //6 01000001
      {68, 68, 68, 68, 68, 68, 68, 68},         //7 01000100
      {0, 0, 0, 8, 0, 0, 0, 8},                 //8 00000000000000000000000000001000
      {255, 255, 255, 255, 255, 255, 255, 255}, //9 1111
      {21, 21, 21, 12, 21, 21, 21, 21},         //10 00010101
      {2, 2, 2, 2, 2, 2, 2, 2}                  //11 00000010
  };

  byte instr4patterntable[G_INTERNALINSTR4PATTERNS + 1 + G_MAXEUCLIDIANPATTERNS][G_MAXBLOCKS] = {
      {0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0},
      {8, 8, 8, 8, 8, 8, 8, 8},                 //2 00001000
      {162, 162, 162, 162, 162, 162, 162, 162}, //3 10100010
      {34, 34, 34, 34, 34, 34, 34, 34},         //4 00100010
      {170, 170, 170, 170, 170, 170, 170, 170}, //5 10101010
      {255, 255, 255, 255, 255, 255, 255, 255}, //6 11111111
      {0, 1, 0, 1, 0, 1, 0, 1},                 //7 00000000 00000001
      {32, 32, 32, 32, 32, 32, 32, 32},         //8 00100000
      {85, 85, 85, 85, 85, 85, 85, 85},         //9 01010101
      {296, 296, 296, 356, 296, 296, 296, 356}, //10 100101000 100101000 100101000 101100100
  };

  byte instr5patterntable[G_INTERNALINSTR5PATTERNS + 1 + G_MAXEUCLIDIANPATTERNS][G_MAXBLOCKS] = {
      {0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0},
      {255, 255, 255, 255, 255, 255, 255, 255}, //2 11111111
      {34, 34, 34, 34, 34, 34, 34, 34},         //3 00100010
      {162, 162, 162, 162, 162, 162, 162, 162}, //4 10100010
      {37, 82, 37, 82, 37, 82, 37, 82},         //5 00100101 01010010
      {8, 8, 8, 8, 8, 8, 8, 8},                 //6 00001000
      {17, 17, 17, 17, 17, 17, 17, 17},         //7 00010001
      {170, 170, 170, 170, 170, 170, 170, 170}, //8 10101010
      {221, 221, 221, 221, 221, 221, 221, 221}, //9 11011101
      {0, 1, 0, 1, 0, 1, 0, 1},                 //10 00000000 00000001
      {1, 1, 1, 1, 1, 1, 1, 0},                 //11 11111110
  };

  byte instr6patterntable[G_INTERNALINSTR6PATTERNS + 1 + G_MAXEUCLIDIANPATTERNS][G_MAXBLOCKS] = {
      {0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0},
      {170, 170, 170, 170, 170, 170, 170, 170}, //2 10101010
      {37, 82, 37, 82, 37, 82, 37, 82},         //3 00100101 01010010
      {162, 162, 162, 162, 162, 162, 162, 162}, //4 10100010
      {4, 4, 4, 4, 4, 4, 4, 4},                 //5 00000100
      {255, 255, 255, 255, 255, 255, 255, 255}, //6 11111111
      {0, 8, 0, 8, 0, 8, 0, 8},                 //7 00000000 00001000
      {0, 48, 0, 56, 0, 48, 0, 58},             //8 00000000 00110000 00000000 00111000 00000000 00110000 00000000 00111010
      {34, 34, 34, 34, 34, 34, 34, 34},         //9 00100010
      {128, 0, 0, 0, 128, 0, 0, 0},             //10 10000000 00000000 00000000 00000000
      {146, 82, 210, 82, 146, 82, 210, 82},     //11 10010010 01010010 11010010 01010010 10010010 01010010 11010010 01010010
      {85, 93, 85, 93, 85, 93, 85, 93},         //12 01010101 01011101
      {0, 0, 0, 93, 0, 0, 0, 93},               //13 00000000 00000000 00000000 00100000
  };

  byte euclidpatterntable[G_MAXEUCLIDIANPATTERNS][G_MAXBLOCKS] = {
      {182, 219, 109, 182, 219, 109, 182, 218},
      {170, 170, 170, 170, 170, 170, 170, 170},
      {148, 165, 41, 74, 82, 148, 165, 40},
      {181, 173, 107, 90, 214, 181, 173, 106},
      {149, 42, 84, 169, 82, 165, 74, 148},
      {148, 148, 148, 148, 148, 148, 148, 148},
      {181, 106, 213, 171, 86, 173, 90, 180},
      {149, 74, 165, 82, 169, 84, 170, 84},
      {148, 146, 146, 82, 74, 73, 73, 40},
      {190, 251, 239, 190, 251, 239, 190, 250},
      {187, 118, 237, 219, 183, 110, 221, 186},
      {181, 181, 181, 181, 181, 181, 181, 180},
      {181, 90, 173, 86, 171, 85, 170, 212},
      {149, 82, 170, 85, 74, 169, 85, 42},
      {149, 41, 82, 149, 41, 82, 149, 40},
      {137, 36, 137, 36, 137, 36, 137, 36},
      {191, 191, 191, 191, 191, 191, 191, 190},
      {181, 107, 86, 181, 107, 86, 181, 106},
      {149, 82, 149, 82, 149, 82, 149, 82},
      {181, 86, 181, 86, 181, 86, 181, 86},
      {149, 85, 42, 149, 85, 42, 149, 84},
      {181, 85, 106, 181, 85, 106, 181, 84},
  };

  // boolean booleanEuclid[G_MAXEUCLIDIANPATTERNS][G_MAXSTEPS] = {
  //     {1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0},
  //     {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
  //     {1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0},
  //     {1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0},
  //     {1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0},
  //     {1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0},
  //     {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0},
  //     {1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0},
  //     {1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0},
  //     {1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0},
  //     {1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0},
  //     {1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0},
  //     {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0},
  //     {1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0},
  //     {1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0},
  //     {1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0},
  //     {1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0},
  //     {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0},
  //     {1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0},
  //     {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0},
  //     {1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0},
  //     {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0}
  // };

  //https://forum.arduino.cc/index.php?topic=642706.0
  //easy acessible by :
  //steps[instrument][pattern][step];
  byte (*blocks[6])[G_MAXBLOCKS] = {instr1patterntable, instr2patterntable, instr3patterntable, instr4patterntable, instr5patterntable, instr6patterntable};
  //or
  //byte *steps[G_MAXINSTRUMENTS] = {&instr1[0][0], &instr2[0][0], &instr3[0][0], &instr4[0][0], &instr5[0][0], &instr6[0][0]};
  //return steps[_instr][PatternPattern[_instr]->getValue() * G_MAXSTEPS + _step];
  //legacy
  //byte (*steps[6])[G_MAXINSTRUMENTS] = {instr1patterntable, instr2patterntable, instr3patterntable, instr4patterntable, instr5patterntable, instr6patterntable};
};

#endif
