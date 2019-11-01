/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create Melodys
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/
#ifndef Melody_h
#define Melody_h

#include "Arduino.h"
#include "PantalaDefines.h"
#include "Filter.h"

#define MAXMELODYPARMS 6
#define PARAMKEY 0      //key
#define PARAMSPR 1      //spread
#define PARAMACC 2      //accent
#define PARAMINH 3      //inheritance
#define PARAMFIL 4      //filter
#define PARAMSIZ 5      //size
#define MAXREADSCALE 48 //

class Melody
{
public:
  Melody(uint8_t maxsteps);
  boolean updateParameters(uint8_t _param, uint8_t _val);
  uint16_t getNote();
  void resetStepCounter();
  void computeNewMelody();

private:
  void calculateAccents();
  uint8_t applyFilter(uint8_t _note);
  uint8_t stepCounter; //melody step counter
  //uint8_t loopCounter;
  //uint8_t loopArray[5] = {4, 8, 16, 32, 64};
  //2=final original calculated
  //1=final complete hith inheritance
  //0=final with calculated loops
  int8_t initialMelody[64];
  int8_t finalMelody[64];
  int8_t accent[64];
  int8_t lastRead[MAXMELODYPARMS];
  int8_t parameters[MAXMELODYPARMS][4] = {
      //min , max, final wheighted , multiplier
      {0, 11, 0, 1},   //"key",           //*
      {0, 11, 0, 1},   //"spread",     //*
      {0, 6, 0, 1},    //"accents"         //**
      {1, 10, 50, 10}, //"inheritance",   //*
      {1, 15, 30, 4},  //"filter"         //**
      {0, 4, 2, 1}     //"size",          //**
  };
  uint8_t _maxsteps;
  uint8_t _lastPlayedNote;
  uint8_t baseNote = 24;
};

#endif
