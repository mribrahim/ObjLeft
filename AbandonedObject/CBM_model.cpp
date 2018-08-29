#include "CBM_model.h"
#include <iostream>
#include <omp.h>

using namespace std;
using namespace cv;

CBM_model::CBM_model(myImage * input, int set_MOG_LearnFrame, int set_min_area, int set_buffer_len, float set_resize, myImage * mask)
{
	blob_detect = new Blob_Detect(MIN_SFG);
	frame_count = 0;
	sampling_idx = 0;
	FG_count = 0;
	RESIZE_RATE = set_resize;

	MOG_LEARN_FRAMES = set_MOG_LearnFrame;
	MIN_AREA = set_min_area;
	TEMPORAL_RULE = set_buffer_len;

	new_width = (int)(input->width);
	new_height = (int)(input->height);

	Initialize();

	// Select parameters for Gaussian model.
	_myGMM = new myGMM(0.0001);//0.0001
	_myGMM2 = new myGMM(0.002);

	maskROI = mask;

}

CBM_model::~CBM_model()
{
	Uninitialize();
}
void CBM_model::Initialize()
{

#ifdef WRITER_DEF
	_writer1 = cvCreateVideoWriter("long.avi",CV_FOURCC('X','V','I','D'),30,cvSize(new_width,new_height),0);
	_writer2 = cvCreateVideoWriter("short.avi",CV_FOURCC('X','V','I','D'),30,cvSize(new_width,new_height),0);
	_writer3 = cvCreateVideoWriter("static.avi",CV_FOURCC('X','V','I','D'),30,cvSize(new_width,new_height),1);
	_writer5 =  cvCreateVideoWriter("DPM.avi",CV_FOURCC('X','V','I','D'),30,cvSize(new_width,new_height),1);
#endif


	mog_fg = cvCreateImage(cvSize(new_width, new_height), IPL_DEPTH_8U, 1);
	mog_fg2 = cvCreateImage(cvSize(new_width, new_height), IPL_DEPTH_8U, 1);
	imgStatic = cvCreateImage(cvSize(new_width, new_height), IPL_DEPTH_8U, 3);

	my_mog_fg = myCreateImage( new_width, new_height, 1);
	my_mog_fg2 = myCreateImage( new_width, new_height, 1);
	my_imgCandiStatic = myCreateImage( new_width, new_height, 3);
	my_imgStatic = myCreateImage( new_width, new_height, 3);

	imageFSM = (pixelFSM **)malloc((int)new_width*sizeof( pixelFSM *));

	cvZero(mog_fg);
	cvZero(mog_fg2);
	cvZero(imgStatic);

	input_temp = myCreateImage(new_width, new_height, 1);		

	for (int i = 0; i < new_width; i++){
		imageFSM[i] = (pixelFSM *)malloc((int)new_height*sizeof(pixelFSM));
	}

	Previous_FG = (bool ***)malloc(TEMPORAL_RULE*sizeof(bool **));
	for (int i = 0; i < TEMPORAL_RULE; i++){
		Previous_FG[i] = (bool **)malloc((int)new_width*sizeof(bool *));
	}
	for (int i = 0; i < TEMPORAL_RULE; i++){
		for (int j = 0; j < new_width; j++){
			Previous_FG[i][j] = (bool *)malloc((int)new_height*sizeof(bool));
		}
	}


	printf("..\n");
	_Previous_Img = (myImage **)malloc(TEMPORAL_RULE*sizeof(myImage *));
	for ( int i = 0; i < TEMPORAL_RULE; i++){
		_Previous_Img[i] = myCreateImage( new_width, new_height, 3);			
	}
	printf("....\n");

	staticFG_pixel_num_now = -1;
	staticFG_pixel_num_pre = -2;
	staticFG_pixel_num_pre2 = -3;

	dpm_gray = myCreateImage(new_width, new_height, 1);
}
void CBM_model::Uninitialize()
{

#ifdef WRITER_DEF
	cvReleaseVideoWriter(&_writer1);
	cvReleaseVideoWriter(&_writer2);
	cvReleaseVideoWriter(&_writer3);
	cvReleaseVideoWriter(&_writer5);
#endif
	
	cvReleaseImage(&mog_fg);
	cvReleaseImage(&mog_fg2);
	cvReleaseImage(&imgStatic);

	my_mog_fg->myReleaseImage();
	my_mog_fg2->myReleaseImage();
	my_imgCandiStatic->myReleaseImage();
	my_imgStatic->myReleaseImage();
	input_temp->myReleaseImage();

	for (int i = 0; i < TEMPORAL_RULE; i++){
		_Previous_Img[i]->myReleaseImage();
	}

	free(*imageFSM);
	free(imageFSM);
	free(**Previous_FG);
	free(*Previous_FG);
	free(Previous_FG);
	cout<<"CBM_model Released!"<<endl;
}

void CBM_model::System_Reset()
{
#pragma omp parallel for
	for (int i = 0; i < new_width; i++){
		for (int j = 0; j < new_height; j++){
			imageFSM[i][j].state_now = 0;
			imageFSM[i][j].staticFG_stable = false;
			imageFSM[i][j].staticFG_candidate = false;
			imageFSM[i][j].static_count = 0;
		}
	}
	static_object_result.clear();
}

bool CBM_model::Motion_Detection(myImage *img)
{
 	myResize(img, _Previous_Img[FG_count]);

	if( frame_count < MOG_LEARN_FRAMES){
		printf("update mog %d\n",MOG_LEARN_FRAMES-frame_count);

		if (frame_count==0){
			_myGMM->initial(_Previous_Img[FG_count]);
			_myGMM2->initial(_Previous_Img[FG_count]);
		}
		_myGMM->process(_Previous_Img[FG_count],my_mog_fg);
		_myGMM2->process(_Previous_Img[FG_count],my_mog_fg2);

		frame_count++;

#ifdef WRITER_DEF
		cvWriteFrame( _writer1, mog_fg);
		cvWriteFrame( _writer2, mog_fg2);
		cvWriteFrame( _writer3, imgStatic);
#endif
		return false;
	}
	else{
		
		//printf("start detect\n");
		//***MOG model***//
		_myGMM->process(_Previous_Img[FG_count],input_temp);
		myImageAND(input_temp,maskROI,my_mog_fg);

		_myGMM2->process(_Previous_Img[FG_count],input_temp);
		myImageAND(input_temp,maskROI,my_mog_fg2);

		myDiladeitself( my_mog_fg, 3);
		myDiladeitself( my_mog_fg2, 3);

		if (check_foreground2(my_mog_fg)>(my_mog_fg->width*my_mog_fg->height*0.30)){//if motion detection cannot work well
 			_myGMM->ChangeLearningRate(0.02);//speed up long-term model's learning rate to adapt the lighting changes.
		}
		else{
			_myGMM->ChangeLearningRate(0.0001);//defult long-term model learning rate
		}

		myFSM( my_mog_fg2, my_mog_fg, imageFSM, Previous_FG);	
		myConvertFSM2Img( imageFSM, my_imgCandiStatic, my_imgStatic);

		staticFG_pixel_num_pre2 = staticFG_pixel_num_pre;
		staticFG_pixel_num_pre = staticFG_pixel_num_now;
		staticFG_pixel_num_now = check_foreground2(my_imgStatic);

 		myImage_2_opencv(my_imgStatic,imgStatic);
		myImage_2_opencv(my_mog_fg,mog_fg);
		myImage_2_opencv(my_mog_fg2,mog_fg2);

		//Mat mog1(mog_fg);
		//Mat mog2(mog_fg2);
		//Mat dest;
		//absdiff(mog1, mog2, dest);
		//imshow("Difference", dest);

		IplImage *candidateStatic = cvCreateImage(cvSize(new_width, new_height), 8, 3);
		myImage_2_opencv(my_imgCandiStatic, candidateStatic);
		

		bool static_object_detected = false;
		//if((staticFG_pixel_num_now==staticFG_pixel_num_pre)&&(staticFG_pixel_num_pre==staticFG_pixel_num_pre2)&&(staticFG_pixel_num_now>0))
		//	static_object_detected = myClustering2( my_imgStatic, 1);

		//IplImage *temp_imgStatic = cvCreateImage(cvSize(new_width, new_height), 8, 3);;

		Mat tempMat = cv::cvarrToMat(imgStatic);
		cvtColor(tempMat, tempMat, CV_BGR2GRAY);

		myblobs = blob_detect->FindBlobs(tempMat);
		if (myblobs.size() > 0) {
			static_object_detected = true;

			std::list<Blob>::iterator it = myblobs.begin();
			for (size_t i = 0; i < myblobs.size(); i++)
			{
				Obj_info * element;
				element = new Obj_info;
				element->x = it->bounding_box.x;
				element->y = it->bounding_box.y;
				element->width = it->bounding_box.width;
				element->height = it->bounding_box.height;
				static_object_result.push_back(element);
		}

		}

#ifdef WRITER_DEF
		cvWriteFrame( _writer1, mog_fg);
		cvWriteFrame( _writer2, mog_fg2);
		cvWriteFrame( _writer3, imgStatic);
#endif

		FG_count = FG_count + 1;
		FG_count = FG_count%TEMPORAL_RULE;

		cvShowImage("Candidate static obj", candidateStatic);
		cvShowImage("static obj", imgStatic);
		cvShowImage("Long-term", mog_fg);
		cvShowImage("Short-term", mog_fg2);
		cvWaitKey(1);

		
		return static_object_detected;
	}
}


bool CBM_model::myClustering2( myImage *img, int option)
{
	int area_threshold = 0;
	myImage * temp;
	temp = myCreateImage(new_width, new_height, 1);
	if (img->depth==3)//static foreground object
	{
		myRGB2Gray(img, temp);
		area_threshold = MIN_AREA/2;//0;
	}
	else if (img->depth==1)//foreground detection
	{
		myImageCopy( img, temp);
		area_threshold = MIN_AREA;
	}
	
	int found_objnum = 0;

	found_objnum = GetLabeling2( temp, area_threshold, option);
	//printf("myClustering: found_objnum=%d\n",found_objnum);

	temp->myReleaseImage();
	delete(temp);

	if (found_objnum > 0){
		return true;
	}
	else{
		return false;
	}
}

/************************************************************************/
/*                                                                      
GetLabeling : input a binary frame, bounding the connected component.
Ignore the connected component when :  case1.  It's pixel is more than a areaThreshold.
                                       case2.  The bounding rectangle is too thin or fat.  */
/************************************************************************/
int CBM_model::GetLabeling2( myImage *pImg1, int areaThreshold, int option) 
{   
    int	found_objnum = 0; 
	if ( option == 0)
		detected_result.clear();//clear the vector
	if ( option == 1)
		static_object_result.clear();//clear the vector

    //find object's conturs of binary frame 
    unsigned int *out = (unsigned int *)malloc(sizeof(*out)* pImg1->width*pImg1->height);

	for (int i = 0; i < pImg1->width*pImg1->height; i++){
		out[i] = pImg1->pixelData[i];
	}

    ConnectedComponents cc(30);
    cc.connected(pImg1->pixelData, out, pImg1->width, pImg1->height,
					std::equal_to<unsigned char>(),
					constant<bool,true>());

// 	myImage * temp;
// 	temp = myCreateImage(pImg1->width, pImg1->height, 3);
// 	IplImage * ttt = cvCreateImage(cvSize(pImg1->width, pImg1->height), IPL_DEPTH_8U, 3);
// 	for (int i = 0; i < pImg1->width*pImg1->height; i++){
// 		temp->pixelData[i] = out[i]*20;
// 		temp->pixelData[i + pImg1->width*pImg1->height] = out[i]*20;
// 		temp->pixelData[i + pImg1->width*pImg1->height*2] = out[i]*20;
// 	}
	bool constant_template[256]={false};
	vector<int> color_labels;
	color_labels.clear();
	for (int i = 0; i < pImg1->width*pImg1->height; i++){
		constant_template[out[i]] = true;
	}
	for (int i = 0; i < 256; i++){
		if (constant_template[i]==true){
			found_objnum++;
			color_labels.push_back(i);
		}
	}

// 	myImage_2_opencv(temp,ttt);
// 	cvShowImage("fast connected",ttt);
// 	cvWaitKey(1);
// 	delete(temp);
// 	cvReleaseImage(&ttt);

	if ( found_objnum == 1){
		free(out);
		return found_objnum-1;
	} 
	else{
		for (int n = 0; n < found_objnum; n++)
		{
			int blob_x1 = pImg1->width, blob_y1 = pImg1->height, blob_x2 = 0, blob_y2 = 0;
			for (int i = 0; i < pImg1->width; i++){
				for (int j = 0; j < pImg1->height; j++){
					if (out[i+j*pImg1->width] == color_labels.at(n)){
						if (i<blob_x1)  blob_x1 = i;
						if (j<blob_y1)  blob_y1 = j;
						if (i>blob_x2)  blob_x2 = i;
						if (j>blob_y2)  blob_y2 = j;
					}
				}
			}
			int blob_w = 0, blob_h = 0;
			blob_w = (blob_x2 - blob_x1)+1;
			blob_h = (blob_y2 - blob_y1)+1;

			//rectangle ratio filter
			int areaThreshold_max = 0, areaThreshold_min = 0;
			if (option==0)//for moving foreground
			{
				areaThreshold_max = MAX_FG;
				areaThreshold_min = MIN_FG;
			}
			else if (option==1)
			{
				areaThreshold_max = MAX_SFG;
				areaThreshold_min = MIN_SFG;
			}

            if(  (  ( (int)blob_w*(int)blob_h) > areaThreshold_min  ) && 
                 (  ( (int)blob_w*(int)blob_h) < (float)areaThreshold_max  ) )   
			{  
				Obj_info * element;
				element = new Obj_info;
				element->x = blob_x1 + blob_w/2;
				element->y = blob_y1 + blob_h/2;
				element->width = blob_w;
				element->height = blob_h;
				//cvRectangle( img, cvPoint(blob_x1,blob_y1), cvPoint(blob_x2,blob_y2), CV_RGB(255,255,255), 2, 8, 0);

				if ( option == 0)	detected_result.push_back( element );
				if ( option == 1)	static_object_result.push_back( element );
            }//end of filter  
		}
		free(out);
		return found_objnum-1; 
	}//end of object checking
}


vector<Obj_info*> CBM_model::GetDetectResult()
{
	return detected_result;
}
vector<Obj_info*> CBM_model::GetStaticForegourdResult()
{
	return static_object_result;
}

myImage * CBM_model::_GetObjLfetFrame()
{
	return _Previous_Img[(FG_count+TEMPORAL_RULE/2)%TEMPORAL_RULE];
}
myImage * CBM_model::_GetPreviousFrame()
{
	return _Previous_Img[FG_count];
}
myImage * CBM_model::_GetPrevious_nFrame( int n)
{
	return _Previous_Img[(FG_count+(TEMPORAL_RULE-n))%TEMPORAL_RULE];
}
myImage * CBM_model::_GetStaticForeground()
{
	return my_imgStatic;
}
bool ** CBM_model::GetPrevious_nForeground( int n)
{
	return Previous_FG[(FG_count+(TEMPORAL_RULE-n))%TEMPORAL_RULE];
}


void CBM_model::myFSM(myImage *short_term, myImage *long_term, pixelFSM ** imageFSM, bool *** Previous_FG)
{
	myColor buffer[2];
	#pragma omp parallel for
	for (int i = 0; i < new_width; i++){
		for (int j = 0; j < new_height; j++){
			buffer[0] = myGet2D(short_term,i,j);
			buffer[1] = myGet2D(long_term,i,j);

			imageFSM[i][j].state_pre = imageFSM[i][j].state_now;
			imageFSM[i][j].state_now = 0;

			if ((buffer[0].B==255)&&(buffer[0].G==255)&&(buffer[0].R==255)){
				imageFSM[i][j].state_now += 2;
			} 
			else{
				imageFSM[i][j].state_now = 0;
			}

			if ((buffer[1].B==255)&&(buffer[1].G==255)&&(buffer[1].R==255)){
				imageFSM[i][j].state_now++;
			} 
			else{
				imageFSM[i][j].state_now = 0;
			}

			if ((imageFSM[i][j].state_now==1)&&(imageFSM[i][j].state_pre==1)){			
				if (imageFSM[i][j].static_count==(TEMPORAL_RULE/2)){
					imageFSM[i][j].staticFG_stable = true;
				}			
				
				if (imageFSM[i][j].staticFG_candidate == true){
					imageFSM[i][j].static_count++;
				}
			}
			else
			{
				imageFSM[i][j].static_count = 0;
				imageFSM[i][j].staticFG_candidate = false;
			}

			if ((imageFSM[i][j].state_now==1)&&(imageFSM[i][j].state_pre==3))
			{
				imageFSM[i][j].staticFG_candidate = true;
			}

			if (imageFSM[i][j].state_now==3)
				Previous_FG[FG_count][i][j] = true;
			else
				Previous_FG[FG_count][i][j] = false;
		}
	}
}

void CBM_model::myConvert2Img(bool **Array, myImage *output)
{
	#pragma omp parallel for
	for (int i = 0; i < new_width; i++){
		for (int j = 0; j < new_height; j++){
			if ( Array[i][j] == true ){
				myColor a; a.B = 255; a.G = 0; a.R = 0;
				mySet2D( output, a, i, j);
			}
			else{
				myColor a; a.B = 0; a.G = 0; a.R = 0;
				mySet2D( output, a, i, j);
			}
		}
	}
}

void CBM_model::myConvertFSM2Img(pixelFSM **Array, myImage *Candidate_Fg, myImage *Static_Fg )
{
	myColor color1, color2;
	color1.B = 0; color1.G = 0; color1.R = 255;
	color2.B = 0; color2.G = 200; color2.R = 255;
	#pragma omp parallel for	
	for (int i = 0; i < new_width; i++){
		for (int j = 0; j < new_height; j++){
			if ( Array[i][j].staticFG_candidate == true )
				mySet2D(Candidate_Fg,color1,i,j);
			else{
				myColor a; a.B = 0; a.G = 0; a.R = 0;
				mySet2D(Candidate_Fg,a,i,j);
			}

			if ( Array[i][j].staticFG_stable == true )
				mySet2D(Static_Fg,color2,i,j);
			else{
				myColor a; a.B = 0; a.G = 0; a.R = 0;
				mySet2D(Static_Fg,a,i,j);	
			}
		}
	}
}


int CBM_model::check_foreground2( myImage * img)
{
	int foregroud = 0;
	myColor a;
	for (int i = 0; i < img->width; i++)
	{
		for (int j = 0; j < img->height; j++)
		{
			a = myGet2D(img, i, j);
			if ((a.B >= 100)||(a.G >= 100)||(a.R >= 100))
			{
				foregroud++;
			}
		}
	}
	return foregroud;
}
