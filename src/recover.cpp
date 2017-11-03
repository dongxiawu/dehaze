#include "recover.h"
#include <algorithm>

#include "guidedfilter.h"

using namespace std;
using namespace cv;

cv::Mat recover(const cv::Mat& src, const cv::Mat& transmission, Vec3f atmosphericLight, double t0){
	CV_Assert(src.channels() == 3 && src.depth() == CV_32F);
	vector<Mat> channels;
	split(src,channels);

//	Mat t = max(t0,transmission);
	Mat t = transmission;
	channels[0] = (channels[0]-atmosphericLight[0])/t + atmosphericLight[0];
	channels[1] = (channels[1]-atmosphericLight[1])/t + atmosphericLight[1];
	channels[2] = (channels[2]-atmosphericLight[2])/t + atmosphericLight[2];

	Mat recover;
	merge(channels,recover);

	return recover;
}