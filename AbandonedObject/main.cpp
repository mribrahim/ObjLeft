#include "ObjLeftDetect.h"
#include "VideoDetails.h"
#include "parameter.h"
#include <iostream>
using namespace std;

int OWNER_SEARCH_ROI;			
int GMM_LEARN_FRAME;//frame number for GMM initialization		
int MAX_SFG;					
int MIN_SFG;					
int MAX_FG;						
int MIN_FG;						
int BUFFER_LENGTH;				
double GMM_LONG_LEARN_RATE;		
double GMM_SHORT_LEARN_RATE;	
double INPUT_RESIZE;	



int64 work_begin;
double work_fps;

int roipointnumber = 0;	
int arr[1] = {4}; 
CvPoint PointArray1[4];
CvPoint *PointArray[2]= {&PointArray1[0],&PointArray1[2]};
int imageheight;
int imagewidth;
IplImage *mask;
myImage *mymask;
void onMouse(int Event,int x,int y,int flags,void* param );

void onMouse(int Event,int x,int y,int flags,void* param )
{
	if ( (Event == 1) && (roipointnumber <=3) )	
	{
		PointArray[roipointnumber/2][roipointnumber%2] = cvPoint(x,y);	
		printf("( %d, %d)\n ",x,y);
		printf("%d\n",roipointnumber);
		roipointnumber = roipointnumber + 1;
	}
	if (  (Event == 1) && (roipointnumber >3))	
	{
		IplImage *Image1 = cvCreateImage(cvSize(imagewidth,imageheight), IPL_DEPTH_8U, 1);
		cvPolyLine( Image1, PointArray, arr, 1, 5, CV_RGB(255,255,255));	
		cvFillPoly(Image1,PointArray,arr,1,CV_RGB(255,255,255));	
		cvThreshold(Image1,Image1,254,255,CV_THRESH_BINARY);	
		mask = Image1;
		opencv_2_myImage(mask,mymask);
		cvSaveImage("mask.jpg",Image1);	
		cvShowImage("SetROI",Image1);	
		cvWaitKey(0);
		cvReleaseImage(&Image1);
	}
}

inline void WorkBegin() 
{ 
	work_begin = getTickCount(); 
}

inline void WorkEnd()
{
    int64 delta = getTickCount() - work_begin;
    double freq = getTickFrequency();
    work_fps = freq / delta;
}

inline string WorkFps()
{
    stringstream ss;
    ss << work_fps;
    return ss.str();
}

int main(int argc, char*argv[])
{	
	/************************************************************************/
	/* parameter seeting                                                    */
	/************************************************************************/
	int count = 0;
	double readData[10] = {0.0};
	FILE *InputFile;  
	InputFile = fopen( "parameter.txt","r");
	if (InputFile == NULL)
	{
		printf("Loading parameter failed.\n");
		system("pause");
		return -1;
	}
	for (int i = 0; i < 10; i++ ){
		fscanf( InputFile, "%lf", &readData[i]);
	}
	fclose(InputFile);
	for (int i = 0; i < 10; i++)
	{
		switch(i)
		{
			case 0: OWNER_SEARCH_ROI = (int)readData[i]; break;
			case 1: GMM_LEARN_FRAME = (int)readData[i]; break;
			case 2: MAX_SFG = (int)readData[i]; break;
			case 3: MIN_SFG = (int)readData[i]; break;
			case 4: MAX_FG = (int)readData[i]; break;
			case 5: MIN_FG = (int)readData[i]; break;
			case 6: BUFFER_LENGTH = (int)readData[i]; break;
			case 7: GMM_LONG_LEARN_RATE = readData[i]; break;
			case 8: GMM_SHORT_LEARN_RATE = readData[i]; break;
			case 9: INPUT_RESIZE = readData[i]; break;
		}
	}

	/************************************************************************/
	/* choose input channel                                                 */
	/************************************************************************/
	string str = "0";
	char* test_video = (char*)str.c_str();

	int method;


	for (int argv_index = 1; argv_index < argc; argv_index++)
	{
		if (0 == strcmp("-v", argv[argv_index]))
		{
			if ((++argv_index) < argc)
			{
				test_video = argv[argv_index];
			}
		}
		else if (0 == strcmp("-c", argv[argv_index]))
		{
			if ((++argv_index) < argc)
			{
				test_video = argv[argv_index];
			}
		}
	}
	

	

	/************************************************************************/
	/* Video input setting                                                   */
	/************************************************************************/
	VideoDetails *_video;
	_video = new VideoDetails(test_video);
	//_video = new VideoDetails("pets2006_1.avi");
	
	imagewidth = _video->_width*INPUT_RESIZE;
	imageheight = _video->_height*INPUT_RESIZE;

	IplImage *qImg, *myimg;
	_video->_currentFrame = 0;
	cvSetCaptureProperty(_video->_file, CV_CAP_PROP_POS_FRAMES, _video->_currentFrame);
	myimg = cvCreateImage(cvSize(imagewidth, imageheight),8,3);
	cv::Mat mat_myimg(myimg,0);
	myImage * myimg1 = myCreateImage(imagewidth, imageheight,3);

#ifdef WRITER_DEF
	CvVideoWriter *_writer;
	_writer = cvCreateVideoWriter("video.avi",CV_FOURCC('X','V','I','D'),30,cvSize(imagewidth, imageheight),1);
#endif

	/************************************************************************/
	/* ROI setting                                                          */
	/************************************************************************/
	
	IplImage *setroi = cvQueryFrame(_video->_file);
	IplImage *setroi2 = cvCreateImage(cvSize(imagewidth,imageheight),8,3);
	mymask = myCreateImage( imagewidth, imageheight, 3);
	myInverse(mymask,mymask) ;
	cvResize(setroi,setroi2);
	cvShowImage("SetROI",setroi2);
	cvSetMouseCallback("SetROI",onMouse,NULL);	
	cvWaitKey(0);
	cvDestroyWindow("SetROI");	
	
	cout << "ROI selection completed." << endl;
	/************************************************************************/
	/* construct object left class                                         */
	/************************************************************************/
	ObjLeftDetect _objleft(myimg1,GMM_LEARN_FRAME,MIN_FG,BUFFER_LENGTH,mymask);
	

	/************************************************************************/
	/* main loop                                                       */
	/************************************************************************/
	bool obj_left = false;
	while(IplImage *temp = cvQueryFrame(_video->_file))
	{		
		qImg  = cvCreateImage(cvSize(imagewidth, imageheight), 8, 3);
		cvResize(temp,qImg,1);

		WorkBegin();
		cvCopy(qImg,myimg);
		medianBlur( mat_myimg, mat_myimg, 3);
		opencv_2_myImage(myimg,myimg1);//transfer opencv data to myimage data

		/************************************************************************/
		/* abandoned object detection algorithm                                 */
		/************************************************************************/
		ProcessReturn obj_left = _objleft.process(myimg1);

		if (obj_left.alarm ==true)
		{
			cout<<"alarm!!"<<endl;

			for (size_t i = 0; i < obj_left.LeftLocation.size(); i++)
			{
				int x = obj_left.LeftLocation.at(i)->x;
				int y = obj_left.LeftLocation.at(i)->y;
				int w = obj_left.LeftLocation.at(i)->width;
				int h = obj_left.LeftLocation.at(i)->height;
				cout << "LeftLocation [ " << i << " ]-> x:" << x << " y:" << y << " w:" << w << " h:" << h << endl;

				cvRectangle(qImg, cvPoint(x,y), cvPoint(x+w,y+h), cvScalar(0, 255, 0));
			}

			for (size_t i = 0; i < obj_left.blobs.size(); i++)
			{
				std::list<Blob>::iterator it = obj_left.blobs.begin();
				std::advance(it, i);
				int x = it->bounding_box.x;
				int y = it->bounding_box.y;
				int w = it->bounding_box.width;
				int h = it->bounding_box.height;
				cout << "Blobs [ " << i << " ]-> x:" << x << " y:" << y << " w:" << w << " h:" << h << endl;

				cvRectangle(qImg, cvPoint(x, y), cvPoint(x + w, y + h), cvScalar(0, 255, 0));
			}
		}
		WorkEnd();
		Mat _qImg(qImg);
		putText(_qImg, "fps:" + WorkFps(), Point(5, 20), FONT_HERSHEY_SIMPLEX, 0.9, Scalar(255, 100, 0), 2);
		cvShowImage("video",qImg);
#ifdef WRITER_DEF
		cvWriteFrame( _writer, qImg);	
#endif
		//cvWaitKey(1);
		if (waitKey(1) == 27) //wait for 'esc' key press. If 'esc' key is pressed, break loop
		{
			cout << "esc key is pressed by user" << endl;
			break;
		}
	}
	myimg1->myReleaseImage();
	cvReleaseImage(&qImg);
	cvReleaseImage(&myimg);
#ifdef WRITER_DEF
	cvReleaseVideoWriter(&_writer);
#endif
	//system("pause");
	return 0;
}
