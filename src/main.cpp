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

    imshow("source image",src);

    //图片必须为彩色图片
    CV_Assert(!src.empty() && src.channels() == 3);
    //图片归一化
    if (src.type() != CV_32FC3){
        src.convertTo(src,CV_32FC3,1.0/255.0);
    }

    //计算大气光
    Vec3f atmosphericLight = estimateAtmosphericLight(src,filterRadius*2+1,topRatio);

    //计算投射图
    Mat transmission = estimateTransmission(src, atmosphericLight,filterRadius*2+1, omega, eps);

    imshow("transmission map",transmission);

    //恢复图像
    Mat recoverImage = recover(src,transmission,atmosphericLight,t0);

    recoverImage.convertTo(recoverImage,CV_8UC3,255);

    imshow("recover image",recoverImage);

    waitKey(0);
    return 0;
}