//
// Created by dongxia on 17-11-8.
//

#ifndef _DEHAZE_DEHAZE_H
#define _DEHAZE_DEHAZE_H

#include <opencv2/opencv.hpp>
#include <queue>

class DeHaze
{
public:
    DeHaze(int r, double t0, double omega, double eps);//构造函数不能是虚函数
    ~DeHaze() = default;

    cv::Mat imageHazeRemove(const cv::Mat& I);

//    void setVideoFPS(int rate);

    cv::Mat videoHazeRemove(const cv::Mat& I);

private:
    cv::Vec3f estimateAtmosphericLight();
    cv::Mat estimateTransmission();
    cv::Vec3f estimateAtmosphericLightVideo();
    cv::Mat estimateTransmissionVideo();
    cv::Mat recover();

private:
    int r;
    double t0;
    double omega;
    double eps;


    cv::Mat I;
    cv::Vec3f atmosphericLight;
    cv::Mat transmission;

//    std::queue<cv::Vec3f> atmosphericLightQueue;

    std::vector<cv::Vec3f> atmosphericLightVector;

};

#endif //DEHAZE_DEHAZE_H
