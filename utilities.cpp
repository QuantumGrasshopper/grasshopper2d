#include "utilities.hpp"

using namespace std;

double get_option(int inputN,char *inputV[], const char *was)
	{
	int n;
	char option[20];
	sprintf(option,"-%s",was);
	for (n=1;n<(inputN-1);n++)
		{
		if (strcmp(inputV[n],option)==0)
			return (double) atof(inputV[n+1]);
		}
	return 0;
	}
	
string get_string_option(int inputN,char *inputV[], const char *was)
	{
	int n;
	char option[20];
	sprintf(option,"-%s",was);
	for (n=1;n<(inputN-1);n++)
		{
		if (strcmp(inputV[n],option)==0)
			return inputV[n+1];
		}
	return 0;
	}

bool isAround(double have, double comparewith)
	{	
	if(abs(have-comparewith)/cellSize<=2) return true;
	else return false;
	}
	
double contributionEnergy(double have, double comparewith)
	{
	double contribution=0;
	if(isAround(have,comparewith)) contribution=(1. + cos(PI*(have-comparewith)/cellSize/2.))/4.;
	return contribution;
	} 
	
int xcoord(int gridPoint)
    {
    return gridPoint%gridSize;
    }
    
int ycoord(int gridPoint)
    {
    return gridPoint/gridSize;
    }

pair<double,double> findPosition(int gridPoint)
	{       
    int y = gridPoint/gridSize;
    int x = gridPoint-y*gridSize;
	
	pair<double,double> thisPair(x*cellSize,y*cellSize);
	
	return thisPair;
	}
	
double euclideanDistance(pair<double,double> point1, pair<double,double> point2)
	{
	double x0=point1.first-point2.first; x0=x0*x0;
	double x1=point1.second-point2.second; x1=x1*x1;
	return sqrt(x1+x0);
	}

double euclideanDistance(pair<int,int> point1, pair<int,int> point2)
	{
	double x0=point1.first-point2.first; x0=x0*x0;
	double x1=point1.second-point2.second; x1=x1*x1;
	return cellSize*sqrt(x1+x0);
	}
	
