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

    cv::Mat videoHazeRemove(const cv::Mat& I);

    void setFPS(int fps);

private:
    cv::Vec3f estimateAtmosphericLight();
    cv::Mat estimateTransmission();
    cv::Vec3f estimateAtmosphericLightVideo();
    cv::Mat estimateTransmissionVideo();
    cv::Mat recover();

    void gammaInit(float gmama);
    void gammaCorrection(cv::Mat& image);

private:
    //common
    int r;
    double t0;
    double omega;
    double eps;


    unsigned char look_up_table[256];

    cv::Mat I;
    cv::Vec3f atmosphericLight;
    cv::Mat transmission;

    //video
    cv::Vec3f atmosphericLightSum;
    std::queue<cv::Vec3f> atmosphericLightQueue;
    cv::Mat preI;
    cv::Mat pre_transmission;

    int fps;

};

#endif //DEHAZE_DEHAZE_H
