#pragma once


#include "opencv2/opencv.hpp"

using namespace cv;
using namespace std;

class Blob
{
public:
	float centerX;
	float centerY;
	float area;
	float perimeter;
	float eccentricity;
	float solidity;
	float compactness;
	float angle;

	Rect bounding_box;


	Blob()
	{
		centerX = -1;
		centerY = -1;
		area = -1;
		perimeter = -1;
		eccentricity = -1;
		solidity = -1;
		compactness = -1;
		angle = -1;
	}
};
