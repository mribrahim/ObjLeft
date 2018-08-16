#include "VideoDetails.h"
using namespace std;
VideoDetails::VideoDetails(char* filename)
{
	/*if (strcmp(filename, "")== 0)
	{
		_file  =  cvCaptureFromCAM(CV_CAP_ANY);
		_fps   =  30;
	}*/
	if (strlen(filename) == 1 && (filename[0]>= 48 || filename[0] <=57) )
	{
		unsigned short x = filename[0] - 48;
		_file = cvCaptureFromCAM(x);
		_fps = 30;
	}
	else{
		_file  =  cvCaptureFromFile(filename);
		_fps   =  (int)cvGetCaptureProperty(_file, CV_CAP_PROP_FPS);
	}
	_width =  cvGetCaptureProperty(_file, CV_CAP_PROP_FRAME_WIDTH);
	_height = cvGetCaptureProperty(_file, CV_CAP_PROP_FRAME_HEIGHT);
	_frameNum = (int) cvGetCaptureProperty(_file, CV_CAP_PROP_FRAME_COUNT);
	cvSetCaptureProperty( _file, CV_CAP_PROP_POS_FRAMES, 0 );/* Return to the beginning */
	_frame = cvCreateImage(cvSize(_width,_height), IPL_DEPTH_8U, 3);//kv
	cout<<"video fps = "<<_fps<<endl;
}
VideoDetails::~VideoDetails()
{
	cvReleaseCapture(&_file);
	if (writer_create==true){
		cvReleaseVideoWriter(&writer_frame);
	}
};

void VideoDetails::VideoWriter_Initial(CvVideoWriter **writer ,char* filename, int fps)
{
	int AviForamt = 0;
	//int fps = 60; // or 25 
    CvSize AviSize = cvSize( _width,_height);
	int AviColor = 1; //0: binary  1:color
	*writer = cvCreateVideoWriter( filename,CV_FOURCC('X','V','I','D'),fps,AviSize,AviColor);
	writer_create = true;
}