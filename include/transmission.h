/*
 * transmission.h
 *
 *  Created on: 2017年10月12日
 *      Author: dongxia
 */

#ifndef TRANSMISSION_H_
#define TRANSMISSION_H_

#include <opencv2/opencv.hpp>

cv::Mat estimateTransmission(const cv::Mat& src, cv::Vec3f atmosphericLight,int r, double omega, double eps);
#endif /* TRANSMISSION_H_ */
