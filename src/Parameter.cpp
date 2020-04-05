/*
Taste & Flavor is an Eurorack module sequence steps, play samples and create Melodies
Creative Commons License CC-BY-SA
Taste & Flavor  by Gibran Curtiss Salomão/Pantala Labs is licensed
under a Creative Commons Attribution-ShareAlike 4.0 International License.
*/

#include "tf_Defines.h"
#include "Parameter.h"
#include <Filter.h>

Parameter::Parameter(uint8_t _min, uint8_t _max, uint8_t _wheight, uint8_t _potPin)
{
#if DO_SERIAL == true
  Serial.print("Parameter started.");
#endif
  dampen = new Filter(6);
  potPin = _potPin;
  minValue = _min;
  maxValue = _max;
  wheight = _wheight;
  value = minValue;
  readParameter();
#if DO_SERIAL == true
  Serial.println("Parameter created.");
#endif
}

boolean Parameter::readParameter()
{
  uint16_t read = analogRead(potPin);
  read = dampen->add(read);
  read = map(read, 0, G_MAX10BIT, minValue, (maxValue + 1) * wheight);
  read = min(read, maxValue);
  //if new reading changed from last saved
  if (value != read)
  {
    value = read;
    return true;
  }
  return false;
}

uint8_t Parameter::getValue()
{
  return value;
}
