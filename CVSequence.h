/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create Melodies
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/
#ifndef CVSequence_h
#define CVSequence_h

#include "tf_Defines.h"
#include "Pinout.h"
#include <PantalaDefines.h>
#include <Counter.h>
#include "Parameter.h"

#define MAXREADSCALE 48 //
#define PARAMSPR 0 //spread
#define PARAMKEY 1 //key
#define PARAMACC 2 //accent
#define PARAMLSZ 3 //loop size
#define PARAMSUB 4 //sub melody
#define PARAMSCL 5 //scale
#define PARAMINH 6 //inheritance
#define PARAMFIL 7 //filter

class CVSequence
{
public:
  CVSequence();
  bool readNewParameter();
  uint16_t getNote();
  uint16_t getSubNote();
  void resetStepCounter();
  void computeNewCVSequence();

private:
  Parameter *parm_spread;
  Parameter *parm_key;
  Parameter *parm_accent;
  Parameter *parm_loopsize;
  Parameter *parm_subseq;
  Parameter *parm_scale;
  Parameter *parm_inheritance;
  Parameter *parm_filter;
  Parameter *parm_array[MAXCVSEQPARMS];

  Counter *queuedParameter;
  void computeAccents();
  void computeSubCVSequence();
  int8_t applyFilter(int8_t _note, bool _type);
  uint8_t stepCounter; //CVSequence step counter

  //2=final original computed
  //1=final complete hith inheritance
  //0=final with computed loops
  int16_t initialCVSequence[G_MAXSTEPS];
  int16_t finalCVSequence[G_MAXSTEPS];
  int16_t subCVSequence[G_MAXSTEPS];
  int16_t accent[G_MAXSTEPS];
  int16_t baseNote = 24;
  int16_t _lastPlayedNote = baseNote;
  uint16_t loopArray[5][G_MAXSTEPS] = {
      {0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3},
      {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63}};
};

#endif
