#include "ObjLeftDetect.h"
#include "VideoDetails.h"
#include "parameter.h"
#include <iostream>

#include "Functions.h"

#include "rapidxml-1.13/rapidxml_iterators.hpp""
#include "rapidxml-1.13/rapidxml_print.hpp"

#include "rapidxml-1.13/rapidxml.hpp""
#include "rapidxml-1.13/rapidxml_utils.hpp"

using namespace std;
using namespace rapidxml;


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
		cvShowImage("SetROI",Image1);	
		//cvSaveImage("mask.jpg", mask);
		//cvWaitKey(0);
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

void loadXML()
{

	OWNER_SEARCH_ROI = 50;
	GMM_LEARN_FRAME = 100;
	MAX_SFG = 150;
	MIN_SFG = 99000;
	MAX_FG = MAX_SFG;
	MIN_FG = MIN_SFG;
	BUFFER_LENGTH = 500;
	GMM_LONG_LEARN_RATE = 0.0001;
	GMM_SHORT_LEARN_RATE = 0.002;
	INPUT_RESIZE = 1;

	xml_document<> doc;
	xml_node<> * root_node;
	// Read the xml file into a vector
	ifstream theFile("parameters.xml");
	vector<char> buffer((istreambuf_iterator<char>(theFile)), istreambuf_iterator<char>());
	buffer.push_back('\0');
	// Parse the buffer using the xml file parsing library into doc 
	doc.parse<0>(&buffer[0]);
	// Find our root node
	root_node = doc.first_node("Configuration");
	// Iterate over the brewerys

	if (NULL == root_node)
	{
		cout << "No configuration XML file!!" << endl;
	}
	else {
		for (xml_node<> * params_node = root_node->first_node("param"); params_node; params_node = params_node->next_sibling())
		{
			string paramname = params_node->first_attribute("name")->value();
			string paramvalue = params_node->first_attribute("value")->value();

			if (paramname.find("owner") != std::string::npos) {
				OWNER_SEARCH_ROI = atoi(paramvalue.c_str());
			}
			else if (paramname.find("learning") != std::string::npos) {
				GMM_LEARN_FRAME = atoi(paramvalue.c_str());
			}
			else if (paramname.find("max_object") != std::string::npos) {
				MAX_SFG = atoi(paramvalue.c_str());
			}
			else if (paramname.find("min_object") != std::string::npos) {
				MIN_SFG = atoi(paramvalue.c_str());
			}
			else if (paramname.find("resize") != std::string::npos) {
				INPUT_RESIZE = stod(paramvalue.c_str());
			}
			else if (paramname.find("frame_count") != std::string::npos) {
				BUFFER_LENGTH = atoi(paramvalue.c_str());
			}
		}
	}

	cout << "owner_roi: " << OWNER_SEARCH_ROI << endl;
	cout << "learning_frame_count: " << GMM_LEARN_FRAME << endl;
	cout << "max_object_area: " << MAX_SFG << endl;;
	cout << "min_object_area: " << MIN_SFG << endl;
	MAX_FG = MAX_SFG;
	MIN_FG = MIN_SFG;
	cout << "frame_count_to_determine_static_object: " << BUFFER_LENGTH <<endl;
	cout << "input_resize: " << INPUT_RESIZE << endl;
	GMM_LONG_LEARN_RATE = 0.0001;
	GMM_SHORT_LEARN_RATE = 0.002;
}


int main(int argc, char*argv[])
{	
	
	loadXML();

	Functions functions;

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
	//cvSetCaptureProperty(_video->_file, CV_CAP_PROP_POS_FRAMES, _video->_currentFrame);
	myimg = cvCreateImage(cvSize(imagewidth, imageheight),8,3);
	cv::Mat mat_myimg = cv::cvarrToMat(myimg);
	myImage * myimg1 = myCreateImage(imagewidth, imageheight,3);

#ifdef WRITER_DEF
	CvVideoWriter *_writer;
	_writer = cvCreateVideoWriter("video.avi",CV_FOURCC('X','V','I','D'),30,cvSize(imagewidth, imageheight),1);
#endif

	/************************************************************************/
	/* ROI setting                                                          */
	/************************************************************************/

	Mat frame;
	_video->_file->read(frame);

	IplImage *setroi = cvCloneImage(&(IplImage)frame);
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
	//while(IplImage *temp = cvQueryFrame(_video->_file))
	while(true)
	{		
		_video->_file->read(frame);
		if (frame.empty())
			break;

		IplImage* temp = cvCloneImage(&(IplImage)frame);
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

			std::list<Blob>::iterator it;
			int i = 0;
			for (it = obj_left.blobs.begin(); it != obj_left.blobs.end(); ++it)
			{
				int x = it->bounding_box.x;
				int y = it->bounding_box.y;
				int w = it->bounding_box.width;
				int h = it->bounding_box.height;
				cout << "Blobs [ " << i << " ]-> x:" << x << " y:" << y << " w:" << w << " h:" << h << endl;
				i++;
				cvRectangle(qImg, cvPoint(x, y), cvPoint(x + w, y + h), cvScalar(0, 255, 0));
			}		
		}


		WorkEnd();
		Mat _qImg = cv::cvarrToMat(qImg);

		if (obj_left.alarm == true)
		{
			string str;
			str.append("im_");  
			str.append(functions.current_time()); 
			str.append(".jpg");
			imwrite(str, _qImg);
		}

		//putText(_qImg, "fps:" + WorkFps(), Point(5, 20), FONT_HERSHEY_SIMPLEX, 0.9, Scalar(255, 100, 0), 2);
		cvPolyLine(qImg, PointArray, arr, 1, 5, CV_RGB(255, 0, 255));
		cvShowImage("video",qImg);
#ifdef WRITER_DEF
		cvWriteFrame( _writer, qImg);	
#endif
		//cvWaitKey(1);
		if ( (char)waitKey(10) == 27 ) //wait for 'esc' key press. If 'esc' key is pressed, break loop
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
