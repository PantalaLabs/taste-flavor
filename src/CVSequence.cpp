/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create Melodies
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "tf_Defines.h"
#include <Counter.h>
#include "CVSequence.h"
#include "Parameter.h"

CVSequence::CVSequence()
{
#if DO_SERIAL == true
  Serial.print("CV sequence started.");
#endif

  parm_spread = new Parameter(1, 11, 1, G_CVSEQPARAMPIN0);
  parm_key = new Parameter(0, 11, 1, G_CVSEQPARAMPIN1);
  parm_accent = new Parameter(0, 6, 1, G_CVSEQPARAMPIN2);
  parm_loopsize = new Parameter(0, 4, 1, G_CVSEQPARAMPIN3);
  parm_subseq = new Parameter(0, 6, 1, G_CVSEQPARAMPIN4);
  parm_scale = new Parameter(0, G_MAXSCALES - 1, 1, G_CVSEQPARAMPIN5);
  parm_inheritance = new Parameter(1, 10, 10, G_CVSEQPARAMPIN6);
  parm_filter = new Parameter(1, baseNote, 2, G_CVSEQPARAMPIN7);

  parm_array[0] = parm_spread;
  parm_array[1] = parm_key;
  parm_array[2] = parm_accent;
  parm_array[3] = parm_loopsize;
  parm_array[4] = parm_subseq;
  parm_array[5] = parm_scale;
  parm_array[6] = parm_inheritance;
  parm_array[7] = parm_filter;

  stepCounter = 0;
  queuedParameter = new Counter(MAXCVSEQPARMS - 1);

  randomSeed(analogRead(G_CVSEQPARAMPIN4));
  computeNewCVSequence();
  computeAccents();
  computeSubCVSequence();
#if DO_SERIAL == true
  Serial.println("CV sequence created.");
#endif
}

uint16_t CVSequence::getSubNote()
{
  return subCVSequence[stepCounter];
}

uint16_t CVSequence::getNote()
{
  //final CVSequence + key + accent
  _lastPlayedNote = applyFilter(                                               //
      subCVSequence[stepCounter] +                                             //
          finalCVSequence[loopArray[parm_loopsize->getValue()][stepCounter]] + //
          parm_key->getValue() +                                               //
          accent[stepCounter],
      false);

  //scale note
  _lastPlayedNote = scaleMidiNote(_lastPlayedNote, parm_scale->getValue());

  stepCounter++;
  if (stepCounter >= G_MAXSTEPS)
    stepCounter = 0;

  return _lastPlayedNote;
}

//apply HPF or LPF : MIDI field
int8_t CVSequence::applyFilter(int8_t _note, bool _type)
{
  uint8_t filterCutOff = parm_filter->getValue();
  if (_type)
  {
    //filter A
    //HPF if filterCutOff bigger than 30, example 38 , ignore everything LOWER than 38-30 = 8
    //LPF if filterCutOff smaller than 29, example = 25  , //ignore everything HIGHER than 30+25=50
    if (((filterCutOff > 25) && (_note < (filterCutOff - baseNote))) ||
        ((filterCutOff < 23) && (_note > (baseNote + filterCutOff))))
      return baseNote;
  }
  else
  {
    //filter B
    //HPF if filterCutOff bigger than 30, example 38 , ignore everything LOWER than 38-30 = 8
    if ((filterCutOff > 25) && (_note < (filterCutOff - baseNote)))
      return (filterCutOff - baseNote);
    //LPF if filterCutOff smaller than 29, example = 25  , //ignore everything HIGHER than 30+25=50
    if ((filterCutOff < 23) && (_note > (baseNote + filterCutOff)))
      return (baseNote + filterCutOff);
  }
  return _note;
}

void CVSequence::resetStepCounter(int32_t _ref)
{
  if (_ref > 0)
    stepCounter = 0;
  else
    stepCounter = 1;
}

bool CVSequence::readNewParameter()
{
  uint16_t _param;
  _param = queuedParameter->advance();
  //if new reading changed from last saved
  if (parm_array[_param]->readParameter())
  {
    if (_param == PARAMACC)
      computeAccents();
    else if (_param == PARAMSUB)
      computeSubCVSequence();
    if ((_param == PARAMSPR)) //no other parameter needs to create new CVSequence because they are dinamically computed at getNote() proc
      computeNewCVSequence();
    return true;
  }
  return false;
}

//compute new base sequence : starts with 10bit field and finished into MIDI field
void CVSequence::computeNewCVSequence()
{
  //uint8_t step = 0;
  for (uint8_t step = 0; step < G_MAXSTEPS; step++)
  {
    //compute first basic sequence
    //key + spread / starts at centered on MIDI note baseNote
    initialCVSequence[step] = baseNote + random(-parm_spread->getValue(), parm_spread->getValue());

    //inheritance
    if (chance(parm_inheritance->getValue(), 100))
      finalCVSequence[step] = initialCVSequence[step];
    finalCVSequence[step] = initialCVSequence[step];
  }
  computeAccents();
}

//compute sub melodies : MIDI field
// void CVSequence::computeSubCVSequence()
// {
//   uint8_t subCVSequenceMagicNumbers[7][2] = {{64, 0}, {32, 1}, {16, 1}, {8, 1}, {32, 4}, {16, 4}, {8, 4}}; //{loop size , key range}
//   int8_t lastSection = 0;                                                                                  //starts with 0 to keep first section into KEY parameter
//   int8_t filler = 0;                                                                                       //fills with 0 the first section to keep first section into KEY parameter
//   int8_t lastfiller = filler;                                                                              //last valid filler
//   for (uint8_t step = 0; step < G_MAXSTEPS; step++)
//   {
//     int8_t newSection = step / subCVSequenceMagicNumbers[parm_subseq->getValue()][0];
//     if (lastSection != newSection)
//     {
//       lastSection = newSection;
//       while (lastfiller == filler)
//         filler = random(-subCVSequenceMagicNumbers[parm_subseq->getValue()][1], subCVSequenceMagicNumbers[parm_subseq->getValue()][1]);
//       lastfiller = filler;
//     }
//     subCVSequence[step] = filler;
//   }
// }

//compute sub melodies : MIDI field
void CVSequence::computeSubCVSequence()
{
  //{group quantity, steps per group , key range}
  uint8_t rules[7][3] = {{1, 64, 0}, {2, 32, 1}, {4, 16, 1}, {8, 8, 1}, {2, 32, 4}, {4, 16, 4}, {8, 8, 4}};
  int8_t filler = 0;     //fills with 0 the first section to keep first section into KEY parameter
  int8_t lastfiller = 0; //last valid filler
  for (uint8_t group = 0; group < rules[parm_subseq->getValue()][0]; group++)
  {
    for (uint8_t step = 0; step < rules[parm_subseq->getValue()][1]; step++)
      subCVSequence[group * rules[parm_subseq->getValue()][1] + step] = filler;
    if (rules[parm_subseq->getValue()][2] != 0)
      if (group < 4)
      {
        while (lastfiller == filler)
          filler = random(-rules[parm_subseq->getValue()][2], rules[parm_subseq->getValue()][2]);
        lastfiller = filler;
      }
      else
      {
        //last group, change or not the key
        if ((group + 2) == rules[parm_subseq->getValue()][0] && random(1))
        {
          while (lastfiller == filler)
            filler = random(-rules[parm_subseq->getValue()][2], rules[parm_subseq->getValue()][2]);
          lastfiller = filler;
        }
      }
  }
}

//compute eventual accents : MIDI field
void CVSequence::computeAccents()
{
  const uint8_t MINCHANCE = 1;
  const uint8_t MAXCHANCE = 3;
  //accents allowed only after the 8th step
  for (uint8_t step = 0; step < G_MAXSTEPS; step++)
  {
    accent[step] = 0;
    switch (parm_accent->getValue())
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