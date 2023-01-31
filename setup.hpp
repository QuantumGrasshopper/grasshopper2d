#include "utilities.hpp"

void initLoad(bool grid[], int spinArray[]);
void initRandom(bool grid[], int spinArray[], gsl_rng* RNG);
void saveConfig(int *spinArray, std::ofstream &filename);
void initialize(bool grid[], int spinArray[], gsl_rng* RNG, std::string initconf);
