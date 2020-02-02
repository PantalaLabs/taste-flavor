#include "Arduino.h"
#include "Melody2.h"
#include <PantalaDefines.h>

Melody2::Melody2()
{
	this->first_iteration = true;
	this->latched_cv_output = 0;
	this->latched_gate_output = 0;
}

uint16_t Melody2::compute(uint8_t _step, uint16_t _pattern_length, uint16_t _cv_pattern, uint16_t _gate_pattern, uint16_t _gate_density)
{
	uint32_t pattern_length = _pattern_length >> CONVERT_TO_6_BIT;
	uint32_t cv_pattern = _cv_pattern >> CONVERT_TO_9_BIT;
	uint32_t gate_pattern = _gate_pattern >> CONVERT_TO_9_BIT;

	if (_step == 0)
	{
		cv_rand.seed(cv_pattern);
		gate_rand.seed(gate_pattern + 10);
	}

	if (first_iteration)
	{
		first_iteration = false;
		latched_cv_output = cv_rand.random();
		if (gate_rand.random() < _gate_density)
			latched_gate_output = MAX_CV;
		else
			latched_gate_output = 0;
	}
	latched_cv_output = cv_rand.random();
	if (gate_rand.random() < _gate_density)
		latched_gate_output = MAX_CV;
	else
		latched_gate_output = 0;
	return (latched_cv_output);
}