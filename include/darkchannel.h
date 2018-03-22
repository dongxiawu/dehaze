//
// Created by dongxia on 17-10-26.
//

#ifndef DEHAZE_DARKCHANNEL_H
#define DEHAZE_DARKCHANNEL_H

#include <opencv2/opencv.hpp>

cv::Mat calcMinChannel(const cv::Mat& src);
cv::Mat calcDarkChannel(const cv::Mat& src, int r);

#endif //DEHAZE_DARKCHANNEL_H
