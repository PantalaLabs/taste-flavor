/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create Melodies
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "tf_Defines.h"
#include "Melody.h"
//#include <PantalaDefines.h>
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

  //scale note
  _lastPlayedNote = quantizeMidiNote(_lastPlayedNote, parameters[PARAMSCL][2]);

  stepCounter++;
  if (stepCounter >= G_MAXSTEPS)
    stepCounter = 0;

  return _lastPlayedNote;
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
  //if new reading changed from last saved
  if (parameters[_param][2] != _read)
  {
    parameters[_param][2] = _read;
    if (_param == PARAMACC)
      computeAccents();
    else if (_param == PARAMSUB)
      computeSubMelody();
    if ((_param == PARAMSPR)) //no other parameter needs to create new melody because they are dinamically computed at getNote() proc
      computeNewMelody();
    return true;
  }
  return false;
}

//compute new base sequence : starts with 10bit field and finished into MIDI field
void Melody::computeNewMelody()
{
  uint8_t step = 0;

  for (uint8_t step = 0; step < G_MAXSTEPS; step++)
  {
    //compute first basic sequence
    //key + spread / starts at centered on MIDI note 24
    initialMelody[step] = 24 + random(-parameters[PARAMSPR][2], parameters[PARAMSPR][2]);

    //inheritance
    if (chance(parameters[PARAMINH][2], 100))
      finalMelody[step] = initialMelody[step];

    finalMelody[step] = initialMelody[step];
  }
  //accents
  computeAccents();
}

//apply HPF or LPF : MIDI field
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

//compute sub melodies : MIDI field
uint8_t subMelodyMagicNumbers[7][2] = {{64, 0}, {32, 1}, {16, 1}, {8, 1}, {32, 4}, {16, 4}, {8, 4}};
void Melody::computeSubMelody()
{
  int8_t lastSection = 200;
  int8_t filler = 0;
  for (uint8_t step = 0; step < G_MAXSTEPS; step++)
  {
    int16_t newSection = step / subMelodyMagicNumbers[parameters[PARAMSUB][2]][0];
    if (lastSection != newSection)
    {
      lastSection = newSection;
      filler = random(-subMelodyMagicNumbers[parameters[PARAMSUB][2]][1], subMelodyMagicNumbers[parameters[PARAMSUB][2]][1]);
    }
    subMelody[step] = filler;
  }
}

//compute eventual accents : MIDI field
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