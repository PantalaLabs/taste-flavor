#ifndef Melody2_h
#define Melody2_h

#include "Arduino.h"
#include "Rand.h"

class Melody2
{

public:
  Melody2();
  uint16_t compute(uint8_t _step, uint16_t _pattern_length, uint16_t _cv_pattern, uint16_t _gate_pattern, uint16_t _gate_density);

private:
  boolean first_iteration;
  uint16_t latched_cv_output;
  uint16_t latched_gate_output;
  Rand cv_rand;
  Rand gate_rand;
};

#endif
