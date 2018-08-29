
#include "Blob_Detect.h"


Blob_Detect::Blob_Detect(int min_blob_size)
{
	// minimum blob size
	blob_size_min = 100; // min_blob_size;
}


Blob_Detect::~Blob_Detect()
{
}


double Blob_Detect::get_eccentricity(Moments mu)
{
	double bigSqrt = sqrt((mu.mu20 - mu.mu02) *  (mu.mu20 - mu.mu02) + 4 * mu.mu11 * mu.mu11);
	return (double)(mu.mu20 + mu.mu02 - bigSqrt) / (mu.mu20 + mu.mu02 + bigSqrt);
}

double Blob_Detect::get_eccentricity2(vector<Point> contour)
{ 
	RotatedRect ellipse = fitEllipse(contour); 
	float a = ellipse.size.height;
	float b = ellipse.size.width;

	if (a<b)
		return  a / b;
	else return b / a;
}

double Blob_Detect::get_solidity(float area, InputArray contour)
{
	vector<Point> ch; convexHull(contour, ch);
	double CHA = contourArea(ch);

	return area / CHA;
}

std::list<Blob> Blob_Detect::FindBlobs(Mat input)
{
	std::list<Blob> myBlobs;

	Mat canny_output;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;


	int width = input.cols;
	int height = input.rows;


	/// Detect edges using canny
	Canny(input, canny_output, 200, 10);
	/// Find contours
	//findContours(canny_output, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
	findContours(canny_output, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE, Point(0, 0));


	for (int i = 0; i< contours.size(); i++)
	{

		if (contourArea(contours[i]) < blob_size_min)
			continue;

		Blob b;

		//mc[i] = Point2f((mu[i].m10 / mu[i].m00), (mu[i].m01 / mu[i].m00));

		//RotatedRect rotatedRect = cv::minAreaRect(contours[i]);
		//cv::Point2f rect_points[4];
		//rotatedRect.points(rect_points);
		//
		//// choose the longer edge of the rotated rect to compute the angle
		//cv::Point2f edge1 = cv::Vec2f(rect_points[1].x, rect_points[1].y) - cv::Vec2f(rect_points[0].x, rect_points[0].y);
		//cv::Point2f edge2 = cv::Vec2f(rect_points[2].x, rect_points[2].y) - cv::Vec2f(rect_points[1].x, rect_points[1].y);

		//cv::Point2f usedEdge = edge1;
		//if (cv::norm(edge2) > cv::norm(edge1))
		//	usedEdge = edge2;

		//cv::Point2f reference = cv::Vec2f(1, 0); // horizontal edge

		//b.angle = 180.0f / CV_PI * acos((reference.x*usedEdge.x + reference.y*usedEdge.y) / (cv::norm(reference) *cv::norm(usedEdge)));

		Rect boundrect = boundingRect(contours[i]);
		b.bounding_box = boundrect;

		//b.center = mc[i];
		b.centerX = boundrect.x + boundrect.width / 2;
		b.centerY = boundrect.y + boundrect.height / 2;

		//b.eccentricity = get_eccentricity(mu[i]);
		//b.eccentricity = get_eccentricity2(contours[i]);

		//b.solidity = get_solidity(b.area,contours[i]);

		b.perimeter = arcLength(contours[i],true);

		b.compactness = b.area / (boundrect.width*boundrect.height);


		myBlobs.push_back(b);

		//break;
	}

	if (myBlobs.size() > 0)
		cout << "blob found!!" << endl;

	return myBlobs;
}