/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create Melodies
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/
#ifndef Parameter_h
#define Parameter_h

#include <PantalaDefines.h>
#include <Filter.h>

class Parameter
{
public:
  Parameter(uint8_t _min, uint8_t _max, uint8_t _wheight, uint8_t _potPin);
  bool readParameter();
  uint8_t getValue();

private:
  Filter *dampen;
  uint8_t potPin;
  uint8_t minValue;
  uint8_t maxValue;
  uint8_t value;
  uint8_t wheight;
  uint8_t computeParameter();
};

#endif
