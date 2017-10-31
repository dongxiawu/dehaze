#include <fastguidedfilter.h>
#include "transmission.h"
#include "darkchannel.h"
#include "guidedfilter.h"

using namespace cv;
using namespace std;

cv::Mat estimateTransmission(const Mat& src, Vec3f atmosphericLight,int r, double omega, double eps){
    CV_Assert(src.channels() == 3);
    vector<Mat> channels;
    split(src,channels);

    channels[0] = channels[0]/atmosphericLight[0];
    channels[1] = channels[1]/atmosphericLight[1];
    channels[2] = channels[2]/atmosphericLight[2];

    Mat normalized;
    merge(channels,normalized);

    Mat darkChannel = calcDarkChannel(normalized,r);

    Mat transmission = 1.0 - omega * darkChannel;

//    Mat gray;
//    cvtColor(src,gray,CV_BGR2GRAY);

    //导向滤波耗时30ms左右
//    transmission = guidedFilter(src, transmission, 8*r, eps);

    transmission = fastGuidedFilter(src, transmission, 8*r, eps);

    return transmission;
}



