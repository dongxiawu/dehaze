//
// Created by dongxia on 17-10-26.
//

#include "darkchannel.h"

using namespace cv;
using namespace std;


cv::Mat calcMinChannel(const cv::Mat& src)
{
    CV_Assert(!src.empty());

    vector<Mat> channels;
    split(src,channels);

    Mat minChannel = channels[0];
    for(size_t i = 1;i<channels.size();i++){
        minChannel = min(minChannel,channels[i]);
    }
    return minChannel;
}

cv::Mat calcDarkChannel(const cv::Mat& src, int r)
{
    Mat minChannel = calcMinChannel(src);

    Mat kernel = getStructuringElement(MORPH_RECT,Size(2*r+1,2*r+1));

    Mat darkChannel;
    erode(minChannel,darkChannel,kernel);

    return darkChannel;
}
