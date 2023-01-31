#include "common.hpp"

using namespace std;

bool isAround(double have, double comparewith)
	{	
	if(abs(have-comparewith)/cellsize<=2) return true;
	else return false;
	}
	
double contributionEnergy(double have, double comparewith)
	{
	double contribution=0;
	if(isAround(have,comparewith)) contribution=(1. + cos(PI*(have-comparewith)/cellsize/2.))/4.;
	return contribution;
	} 
	
int xcoord(int gridPoint)
    {
    return gridPoint%gridSize;
    }
    
int ycoord(int gridPoint)
    {
    //return (gridPoint-xcoord(gridPoint))/gridSize;
    return gridPoint/gridSize;
    }

pair<double,double> findPosition(int gridPoint)
	{
// 	int x=gridPoint%gridSize;
// 	int y=(gridPoint-x)/gridSize;
        
    int y = gridPoint/gridSize;
    int x = gridPoint-y*gridSize;
	
	pair<double,double> thisPair(x*cellsize,y*cellsize);
	
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
	return cellsize*sqrt(x1+x0);
	}
    
