/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create Melodys
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "tf_Defines.h"
#include "Melody.h"
#include <PantalaDefines.h>
#include <Counter.h>

Melody::Melody()
{
  stepCounter = 0;
  queuedParameter = new Counter(MAXMELODYPARMS - 1);
  for (uint8_t i = 0; i < MAXMELODYPARMS; i++)
    filters[i] = new Filter(6);
  randomSeed(analogRead(G_MELODYPARAMPIN4));
  computeNewMelody();
  computeAccents();
  computeSubMelody();
}

//PUBLIC----------------------------------------------------------------------------------------------
uint16_t Melody::getNote()
{
  //final melody + key + accent
  _lastPlayedNote = applyFilter(
      subMelody[stepCounter] +                                       //
      finalMelody[loopArray[parameters[PARAMLSZ][2]][stepCounter]] + //
      parameters[PARAMKEY][2] + accent[stepCounter]);
  //_lastPlayedNote = applyFilter(finalMelody[stepCounter] + accent[stepCounter]);
  _lastPlayedNote = constrain(_lastPlayedNote, 0, 60);

  stepCounter++;
  if (stepCounter >= G_MAXSTEPS)
    stepCounter = 0;

  return NOTES[_lastPlayedNote];
}

void Melody::resetStepCounter()
{
  stepCounter = 0;
}

boolean Melody::readNewMelodyParameter()
{
  uint8_t _param;
  uint16_t _read;
  _param = queuedParameter->advance();
  _read = analogRead(potentiometerPins[_param]);
  _read = filters[_param]->add(_read);
  _read = _read >> 4;
  _read = min(_read, MAXREADSCALE); //crop value to MAXSCALE
  _read = map(_read, 0, MAXREADSCALE, parameters[_param][0], parameters[_param][1] * parameters[_param][3]);
  if (parameters[_param][2] != _read)
  {
    parameters[_param][2] = _read;
    if (_param == PARAMACC)
      computeAccents();
    else if (_param == PARAMSPR)
      computeNewMelody();
    else if (_param == PARAMSUB)
      computeSubMelody();
    return true;
  }
  return false;
}

//PRIVATE----------------------------------------------------------------------------------------------
void Melody::computeNewMelody()
{
  uint8_t step = 0;
  uint8_t lastValidNote = 0;
  // uint8_t stepsize = powint(2, parameters[PARAMSSZ][2]);
  uint8_t stepsize = 1;

  while (step < G_MAXSTEPS)
  {
    //key + spread
    //initialMelody[step] = baseNote + random(-parameters[PARAMSPR][2], parameters[PARAMSPR][2]);
    initialMelody[step] = BASE10BITVOLTAGE + random(-parameters[PARAMSPR][2] * BASE10BITMIDISTEPVOLTAGE, parameters[PARAMSPR][2] * BASE10BITMIDISTEPVOLTAGE);
    initialMelody[step] = map(initialMelody[step], 0, 1023, 0, 60);
    //initialMelody[step] = map10bitAnalog2Scaled5octMidiNote(initialMelody[step], pentaTable);
    //inheritance
    if (chance(parameters[PARAMINH][2], 100))
      finalMelody[step] = initialMelody[step];
    lastValidNote = step;
    // for (int8_t stepfiller = 1; stepfiller < stepsize; stepfiller++)
    // {
    //   step++;
    //   finalMelody[step] = finalMelody[lastValidNote];
    // }
    step++;
  }
  //accents
  computeAccents();
}

int8_t Melody::applyFilter(int8_t _note)
{
  int8_t newNote;
  int8_t filterValue = parameters[PARAMFIL][2];
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

uint8_t subMelodyMagicNumbers[7][2] = {{64, 0}, {32, 1}, {16, 1}, {8, 1}, {32, 4}, {16, 4}, {8, 4}};
void Melody::computeSubMelody()
{
  int8_t lastSection = 0;
  int8_t filler = 0;
  //##############
  for (uint8_t step = 0; step < G_MAXSTEPS; step++)
    subMelody[step] = random(2);
  // for (uint8_t step = 0; step < G_MAXSTEPS; step++)
  // {
  //   int8_t newSection = step / subMelodyMagicNumbers[parameters[PARAMSUB][2]][0];
  //   if (lastSection != newSection)
  //   {
  //     lastSection = newSection;
  //     filler = random(-subMelodyMagicNumbers[parameters[PARAMSUB][2]][1], subMelodyMagicNumbers[parameters[PARAMSUB][2]][1]);
  //   }
  //   subMelody[step] = filler;
  // }
}

void Melody::computeAccents()
{
  uint8_t MINCHANCE = 1;
  uint8_t MAXCHANCE = 3;

  for (uint8_t step = 0; step < G_MAXSTEPS; step++)
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