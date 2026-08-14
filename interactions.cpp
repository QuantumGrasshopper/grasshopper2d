#include "interactions.hpp"
#include "utilities.hpp"

#include <cmath>
#include <stdexcept>

using namespace std;

//grid geometry

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

//grasshopper interaction

bool isAround(double have, double comparewith)
	{
	if(abs(have-comparewith)/cellSize<=2) return true;
	else return false;
	}

double contributionEnergy(double have, double comparewith)
	{
	double contribution=0;

    if(isAround(have,comparewith))
        {
        if(deltaOption==0) contribution=(1. + cos(PI*(have-comparewith)/cellSize/2.))/4.;
        else if(deltaOption==1)
            {
			double absdist=abs(have-comparewith)/cellSize;
            if(absdist<1) contribution=17./48.+sqrt(3.)*PI/108.+absdist/4.-absdist*absdist/4.+(1-2*absdist)*sqrt(1.+12*absdist*(1-absdist))/16.-sqrt(3.)*asin(sqrt(3.)*(2*absdist-1)/2.)/12.;
            else if( (absdist>=1)&&(absdist<2) ) contribution=55./48.-sqrt(3.)*PI/108.-13.*absdist/12.+absdist*absdist/4.+(2*absdist-3)*sqrt(36*absdist-23.-12*absdist*absdist)/48.+sqrt(3.)*asin(sqrt(3.)*(2*absdist-3)/2.)/36.;
            }
        else throw logic_error("Error: Invalid delta function discretization option");
        }

	return contribution;
	}
