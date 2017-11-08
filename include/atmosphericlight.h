/*
 * atmosphericlight.h
 *
 *  Created on: 2017年10月12日
 *      Author: dongxia
 */

#ifndef ATMOSPHERICLIGHT_H_
#define ATMOSPHERICLIGHT_H_

#include <opencv2/opencv.hpp>
cv::Vec3f estimateAtmosphericLight(const cv::Mat& src, int r, double topRatio);
cv::Vec3b EstimationAtmosphericLight(const cv::Mat& src);
cv::Vec3f MyEstimateAtmosphericLight(const cv::Mat& src, int r);
#endif /* ATMOSPHERICLIGHT_H_ */
