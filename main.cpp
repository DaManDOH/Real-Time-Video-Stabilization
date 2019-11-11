// main.cpp
//

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/flann/flann.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/opencv.hpp>

#include "videostab.h"


// This class redirects cv::Exception to our process so that we can catch it and handle it accordingly.
class cvErrorRedirector {
private:
	std::shared_ptr<long long> pFrameNumber;

public:
	int cvCustomErrorCallback() {
		std::cout << "A cv::Exception has been caught. Skipping frame " << *pFrameNumber << "..." << std::endl;
		return 0;
	}

	cvErrorRedirector(std::shared_ptr<long long> frame_number_counter) {
		pFrameNumber = frame_number_counter;
		cv::redirectError((cv::ErrorCallback)cvCustomErrorCallback(), this);
	}
};


/****************************
 * Entry point of execution *
 ****************************/
int main(int argc, char *argv[]) {
	using namespace std::chrono;

	std::string sSourceFilePath;
	if (argc < 2) {
		std::cerr << "ERROR: incorrect number of arguments" << std::endl;
		return -1;
	}
	sSourceFilePath.assign(argv[1]);

	std::string sDestinationFilePath("./com.avi");
	if (argc > 2) {
		sDestinationFilePath.assign(argv[2]);
	}

	bool bShowWindows = false;
	if (argc > 3) {
		std::string sParamTwo(argv[3]);
		std::transform(sParamTwo.begin(), sParamTwo.end(), sParamTwo.begin(), std::tolower);
		bShowWindows = (sParamTwo == "true" || sParamTwo == "1");
	}

	auto pnFrame = std::make_shared<long long>(-1);

	// Redirect exceptions.
	cvErrorRedirector redir(pnFrame);

	//Create a object of stabilization class
	VideoStab stab;

	//Initialize the VideoCapture object
	cv::VideoCapture cap(sSourceFilePath);
	auto nTotalFrameCount = cap.get(CAP_PROP_FRAME_COUNT);
	auto fSourceFPS = cap.get(CAP_PROP_FPS);

	cv::Mat frame_2;
	cv::Mat frame_1;

	auto oStartTime = high_resolution_clock::now();

	cap >> frame_1;
	(*pnFrame)++;

	cv::Mat_<double> smoothedMat(2, 3);

	cv::VideoWriter outputVideo;
	outputVideo.open(
		sDestinationFilePath,
		cv::VideoWriter::fourcc('X', 'V', 'I', 'D'),
		fSourceFPS,
		frame_1.size());

	while (true) {
		cap >> frame_2;
		(*pnFrame)++;

		if (frame_2.data == NULL) {
			break;
		}

		cv::Mat smoothedFrame;

		smoothedFrame = stab.stabilize(frame_1, frame_2, bShowWindows);

		outputVideo.write(smoothedFrame);

		if (bShowWindows) {
			cv::imshow("Stabilized Video", smoothedFrame);
		}

		frame_1 = frame_2.clone();
	}

	auto oEndTime = high_resolution_clock::now();
	auto oRunTIme = duration_cast<duration<double>>(oEndTime - oStartTime);
	std::cout << "\nProcessed frames: "
		<< (*pnFrame) << "/" << nTotalFrameCount
		<< "; Source FPS: " << fSourceFPS
		<< "; Processing FPS: " << ((*pnFrame) / oRunTIme.count()) << std::endl;

	return 0;
}
