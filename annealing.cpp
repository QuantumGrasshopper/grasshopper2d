#include "annealing.hpp"

double temperatureDecrease(double temperature)
	{
	return tempScaling*temperature;
	}
	
unsigned int stepIncrease(unsigned int temproundsteps)
	{
	return int(abs(temproundsteps/tempScaling));
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
