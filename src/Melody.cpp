/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create Melodys
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "tf_Defines.h"
#include "Melody.h"
#include "midi.h"
#include <PantalaDefines.h>

Melody::Melody(uint8_t maxsteps)
{
  _maxsteps = maxsteps;
  stepCounter = 0;
  randomSeed(micros());
  computeNewMelody();
  calculateAccents();
}

//PUBLIC----------------------------------------------------------------------------------------------
uint16_t Melody::getNote()
{
  //final melody + key + accent
  _lastPlayedNote = applyFilter(finalMelody[loopArray[parameters[PARAMSIZ][2]][stepCounter]] + parameters[PARAMKEY][2] + accent[stepCounter]);
  //_lastPlayedNote = applyFilter(finalMelody[stepCounter] + accent[stepCounter]);
  _lastPlayedNote = constrain(_lastPlayedNote, 0, 60);

  stepCounter++;
  if (stepCounter >= _maxsteps)
    stepCounter = 0;

  return mapMidi2Volt(_lastPlayedNote, 0);
}

void Melody::resetStepCounter()
{
  stepCounter = 0;
}

boolean Melody::updateParameters(uint8_t _param, uint16_t _val)
{
  uint16_t read;

  read = min(_val, MAXREADSCALE); //crop value to MAXSCALE
  read = map(_val, 0, MAXREADSCALE, parameters[_param][0], parameters[_param][1] * parameters[_param][3]);
  if (parameters[_param][2] != read)
  {
    // if (_param == PARAMSIZ)
    //   Serial.println(read);
    parameters[_param][2] = read;
    if (_param == PARAMACC)
      calculateAccents();
    else if (_param == PARAMSPR)
      computeNewMelody();
    return true;
  }
  return false;
}

//PRIVATE----------------------------------------------------------------------------------------------
void Melody::computeNewMelody()
{
  for (uint8_t step = 0; step < _maxsteps; step++)
  {
    //key + spread
    //initialMelody[step] = baseNote + random(-parameters[PARAMSPR][2], parameters[PARAMSPR][2]);
    initialMelody[step] = BASE10BITVOLTAGE + random(-parameters[PARAMSPR][2] * BASE10BITMIDISTEPVOLTAGE, parameters[PARAMSPR][2] * BASE10BITMIDISTEPVOLTAGE);
    initialMelody[step] = map10bitAnalog2Scaled5octMidiNote(initialMelody[step], pentaTable);
    //inheritance
    if (chance(parameters[PARAMINH][2], 100))
      finalMelody[step] = initialMelody[step];
  }
  //accents
  calculateAccents();
}

uint8_t Melody::applyFilter(uint8_t _note)
{
  uint8_t newNote;
  uint8_t filterValue = parameters[PARAMFIL][2];
  if (_note > 0) //if there was a note to play .. go ahead or silence
  {              //filter it
    newNote = _note;
    if (filterValue >= 30) //if bigger than 60, example (76 - 60) = 16 cut everything LOWER than 16
    {                      //HPF
      filterValue = filterValue - 30;
      if (newNote <= filterValue)
        newNote = _lastPlayedNote;
    }
    else //if smaller or equal than 60, example = 45 cut everything HIGHER than 45
    {    //LPF
      if (newNote > (filterValue * 2))
        newNote = _lastPlayedNote;
    }
    return newNote;
  }
  return 0;
}

void Melody::calculateAccents()
{
  uint8_t MINCHANCE = 1;
  uint8_t MAXCHANCE = 3;

  for (uint8_t step = 0; step < _maxsteps; step++)
  {
    accent[step] = 0;
    switch (parameters[PARAMACC][2])
    {
    case 0:
      break;
    case 1:
      if (chance(MINCHANCE, 10))
        accent[step] = 12;
      break;
    case 2:
      if (chance(MINCHANCE, 10))
        accent[step] = 24;
      break;
    case 3:
      if (chance(MINCHANCE, 10))
      {
        if (oneChanceIn(2))
          accent[step] = 12;
        else
          accent[step] = 24;
      }
      break;
    case 4:
      if (chance(MAXCHANCE, 10))
        accent[step] = 12;
      break;
    case 5:
      if (chance(MAXCHANCE, 10))
        accent[step] = 24;
      break;
    case 6:
      if (chance(MAXCHANCE, 10))
      {
        if (oneChanceIn(2))
          accent[step] = 12;
        else
          accent[step] = 24;
      }
      break;
    }
    if (oneChanceIn(2))
      accent[step] = -accent[step];
  }
}