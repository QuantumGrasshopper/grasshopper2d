#include "annealing.hpp"
#include "utilities.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

double temperatureDecrease(double temperature)
	{
	//exponential cooling schedule:
	//every annealing round the temperature is multiplied by a constant factor
	return tempScaling*temperature;
	}
	
unsigned long stepIncrease(unsigned long temproundsteps)
	{
	//the number of MC proposals per temperature is increased as cooling proceeds
	//to compensate for the decreasing acceptance rate at lower temperatures
	const long double scaledSteps=static_cast<long double>(temproundsteps)/static_cast<long double>(tempScaling);
	const long double unsignedLongUpperBound=
		std::ldexp(1.0L, std::numeric_limits<unsigned long>::digits);
	if(!std::isfinite(scaledSteps) || scaledSteps>=unsignedLongUpperBound)
		throw std::overflow_error("Temperature-round step count exceeds the unsigned long range.");
	return static_cast<unsigned long>(scaledSteps);
	}

double energyDecreaseProbDistr(double energyDifference, double temperature)
	{
	return exp(energyDifference/temperature);
	}
	
bool acceptreject(double probability, gsl_rng* RNG)
	{
	bool accept;
	double random = gsl_rng_uniform(RNG);
	if(random<abs(probability)){accept=true;}
	else {accept=false;}

	return accept;
	}
