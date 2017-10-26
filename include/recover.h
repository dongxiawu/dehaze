#ifndef RECOVER_H
#define RECOVER_H

#include <opencv2/opencv.hpp>

cv::Mat recover(const cv::Mat& src, const cv::Mat& transmission, cv::Vec3f atmosphericLight, double t0);

#endif

