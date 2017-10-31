#include <iostream>
#include "darkchannel.h"
#include "atmosphericlight.h"
#include "transmission.h"
#include "recover.h"

using namespace std;
using namespace cv;

int main(int argc, char *argv[]) {

    //传入参数没有图片信息，退出
    if(argc < 2){
        cout << "no input image"<<endl;
        return -1;
    }

    const string imgName = argv[1];

    const int filterRadius = 7;

    //top ratio for the brightest pixels
    const double topRatio = 0.001;

    //lower bound t0
    const double t0 = 0.1;

    //keep more haze for the distant objects
    const double omega = 0.95;

    //regularization parameter
    const double eps = 10E-6;

    Mat src = imread(imgName);

    //输出图片大小
    cout<<"图片大小为: "<<src.rows <<"*" << src.cols<<endl;

    //开始计时
    double start =clock();

    //图片必须为彩色图片
    CV_Assert(!src.empty() && src.channels() == 3);
    //图片归一化
    if (src.type() != CV_32FC3){
        src.convertTo(src,CV_32FC3,1.0/255.0);
    }

    //计算大气光
    Vec3f atmosphericLight = estimateAtmosphericLight(src,filterRadius,topRatio);

    //计算投射图
    Mat transmission = estimateTransmission(src, atmosphericLight,filterRadius, omega, eps);

    //恢复图像
    Mat recoverImage = recover(src,transmission,atmosphericLight,t0);

    recoverImage.convertTo(recoverImage,CV_8UC3,255);

    //停止计时
    double stop = clock();

    //输出耗时
    cout<<"恢复图片耗时: "<<(stop-start)/CLOCKS_PER_SEC*1000 <<"ms"<<endl;

    imshow("source image",src);
    imshow("transmission map",transmission);
    imshow("recover image",recoverImage);

    waitKey(0);
    return 0;
}