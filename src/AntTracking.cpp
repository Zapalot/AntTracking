
// Standard include files
#include <opencv2/opencv.hpp>
//#include <opencv2/tracking.hpp>
#include <opencv2/features2d.hpp>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <math.h>

#include "ImagePreprocessor.h"
#include "ContourExtractor.h"
#include "FlannBasedTracker.h"
#include "CoordinateTranslator.h"
#include "ZmqSender.h"
#define liveVideo

using namespace cv;
using namespace std;

// global settings
int camExposure = -6;

//prototypes
VideoCapture setupCamera(int &exposure); //prototype
void createControlWindow(VideoCapture &cam);// prototype
void onExposure(int slider_val, void* cam);

// used to set coordinates in Image
void mouseEvent(int event, int x, int y, int flags, void* userdata) {
	((CoordinateTranslator*)userdata)->mouseEvent(event, x, y, flags, 0);
}

int main(int argc, char **argv)
{
	unsigned long frameIdx=0;
	// set up all object needed
	VideoCapture cam = setupCamera(camExposure);
	int width = cam.get(CV_CAP_PROP_FRAME_WIDTH);
	int height = cam.get(CV_CAP_PROP_FRAME_HEIGHT);

	createControlWindow(cam);
	// these  objects perform all our processing steps:
	ImagePreprocessor bgSubstractor;
	ContourExtractor  contourExtractor(width, height);
	bgSubstractor.setMask(imread(FILE_MASK, CV_LOAD_IMAGE_GRAYSCALE));
	FlannBasedTracker tracker;
	CoordinateTranslator transformer;
	ZmqSender sender;

	contourExtractor.createGui();
	bgSubstractor.createGui();
	tracker.createGui();

	namedWindow("debugOut", CV_WINDOW_AUTOSIZE);
	setMouseCallback("debugOut", mouseEvent, &transformer);

//	createControlWindow(cam);
	cv::Mat inputImage;
	cv::Mat debugOutImage;

	/////////initialize bg
	//take a few frames from the beginning of the file until it has stabelized
	cout << "dropping first frames...\n";
	for (int i = 0; i < 4000; i++) {
		cam.grab();
		frameIdx++;
	}

	cam >> inputImage;
	debugOutImage = inputImage.clone(); //allocate  debugout once
	bgSubstractor.setBackground(inputImage);


	while (true) {
		// get image and process data
		cam>>inputImage;
		frameIdx++;
		inputImage.copyTo(debugOutImage); // we use this as a drawing surface for debug out
		bgSubstractor.processImage(inputImage);
		contourExtractor.extractContours(bgSubstractor.threshedOutput);
		tracker.updateWithContours(contourExtractor.contours);
		sender.sendTrackedObjectData(tracker.trackedObjects, transformer);

		//if(contourExtractor.contours.size())cout << "detected "<<contourExtractor.contours.size()<<"objects in frame"<< frameIdx<<"\n";
		/// Show results in a window
		contourExtractor.showContours( debugOutImage);
		tracker.drawDebugOut(debugOutImage);
		transformer.drawGui(debugOutImage);
		imshow("debugOut", debugOutImage);

		// we need to do this to give opencv time to display the results
		if (cv::waitKey(1) == 27)
			break;
	}
	//startTracking(cam);
}


VideoCapture setupCamera(int &exposure) {


#ifdef liveVideo
	int goodCamIndex = 1;
	/*
	for (int i = 0; i < 10; i++) {
		cv::VideoCapture cam(i);
		cam.set(CV_CAP_PROP_FPS, 30);
		cam.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
		cam.set(CV_CAP_PROP_FRAME_HEIGHT, 720);
		
		if (cam.isOpened()) {
			goodCamIndex = i;
		}
		cam.release();
	}
	*/
	cv::VideoCapture cam(0);
	cam.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M', 'J', 'P', 'G'));
	cam.set(CV_CAP_PROP_FPS, 30);
	cam.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
	cam.set(CV_CAP_PROP_FRAME_HEIGHT, 720);
	cam.set(CV_CAP_PROP_AUTO_EXPOSURE, 0);
	cam.set(CV_CAP_PROP_EXPOSURE, exposure);
	cam.set(CV_CAP_PROP_GAIN, 1);
	cam.set(CV_CAP_PROP_AUTOFOCUS , 0);
	cam.set(CV_CAP_PROP_FOCUS, 1);
	cam.set(CV_CAP_PROP_SETTINGS, 1);
	if (!cam.isOpened()) {
		std::cerr << "no camera detected" << std::endl;
		std::cin.get();
		return 0;
	}
#else
	cv::VideoCapture cam(FILE_VIDEO);
#endif
	return cam;
}

//void createControlWindow(int &set1, int &set2, int &set3, VideoCapture &cam) {
void createControlWindow(VideoCapture &cam) {
	cvNamedWindow("control", WINDOW_NORMAL);

#ifdef liveVideo
	//exposure slider
	int a = cv::createTrackbar("exposure", "control", &camExposure, 5, onExposure, &cam);
#endif
	//background adaption slider
//	int b = cv::createTrackbar("background", "control", &usedBackground, 100, onBackground);
	//brightness slider
}

void measureFramerate(VideoCapture &cam) {
	// std::cout << "measuring fps... ";
	// time_t start, end;
	// time(&start);
	// for (int i = 0; i < 64; i++) {
	// 	cam >> frame;
	// }
	// time(&end);
	// std::cout << 64/difftime(end, start) << "fps" << std::endl;
	// std::cout << "press any key to continue..." << std::endl;
	// std::cin.get();
}


void onExposure(int slider_val, void* cam)
{
	((cv::VideoCapture*)cam)->set(CV_CAP_PROP_EXPOSURE, slider_val - 6);
}

void onBackground(int slider_val, void* cam) {}

void onBrightness(int slider_val, void* cam) {}

