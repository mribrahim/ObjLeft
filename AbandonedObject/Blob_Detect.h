#pragma once


//#include <opencv2/core/core.hpp>
//#include "opencv2/highgui/highgui.hpp"
//#include "opencv2/imgproc/imgproc.hpp"
//#include "opencv2/opencv.hpp"

#include "Blob.h"
#include <vector>

class Blob_Detect
{
public:

	int blob_size_min;

	Blob_Detect(int);
	~Blob_Detect();

	double get_solidity(float,InputArray);
	double get_eccentricity(Moments);
	double get_eccentricity2(vector<Point>);
	list<Blob> FindBlobs(Mat);

	Mat element;
};

