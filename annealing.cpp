#include "annealing.hpp"

double temperatureDecrease(double temperature)
	{
	return tempScaling*temperature;
	}
	
unsigned long stepIncrease(unsigned long temproundsteps)
	{
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
