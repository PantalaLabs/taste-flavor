/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create Melodys
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/
#ifndef Melody_h
#define Melody_h

#include "tf_Defines.h"
#include "Pinout.h"
#include <Arduino.h>
#include <PantalaDefines.h>
#include <Filter.h>
#include <Counter.h>

#define PARAMSPR 0      //spread
#define PARAMKEY 1      //key
#define PARAMACC 2      //accent
#define PARAMINH 3      //inheritance
#define PARAMFIL 4      //filter
#define PARAMSIZ 5      //size
#define PARAMSUB 6      //sub melody
#define MAXREADSCALE 48 //

class Melody
{
public:
  Melody(uint8_t maxsteps);
  boolean readNewMelodyParameter();
  uint16_t getNote();
  void resetStepCounter();
  void computeNewMelody();

private:
  int8_t potentiometerPins[MAXMELODYPARMS] = {G_MELODYPARAMPIN0, G_MELODYPARAMPIN1, G_MELODYPARAMPIN2, G_MELODYPARAMPIN3, G_MELODYPARAMPIN4, G_MELODYPARAMPIN5, G_MELODYPARAMPIN6};
  Counter *queuedParameter;
  Filter *filters[MAXMELODYPARMS]; //add filter for each param pot
  void computeAccents();
  void computeSubMelody();
  int8_t applyFilter(int8_t _note);
  uint8_t stepCounter; //melody step counter
  //2=final original computed
  //1=final complete hith inheritance
  //0=final with computed loops
  int16_t initialMelody[64];
  int16_t finalMelody[64];
  int16_t subMelody[64];
  int16_t accent[64];
  int16_t parameters[MAXMELODYPARMS][4] = {
      //min , max, final wheighted , multiplier
      {0, 11, 0, 1},   //spread
      {0, 11, 0, 1},   //key
      {0, 6, 0, 1},    //accents
      {1, 10, 50, 10}, //inheritance
      {1, 15, 30, 4},  //filter
      {0, 4, 2, 1},    //size
      {0, 6, 5, 1}     //sub melody
  };
  uint8_t _maxsteps;
  uint8_t _lastPlayedNote;
  uint8_t baseNote = 24;
  uint16_t loopArray[5][64] = {
      {0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3},
      {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31},
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63}};
};

#endif
