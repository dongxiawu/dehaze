#include <iostream>
#include "darkchannel.h"
#include "atmosphericlight.h"
#include "transmission.h"
#include "recover.h"

using namespace std;
using namespace cv;

void gammaCorrection(Mat& src, Mat& dst, float fGamma);
Mat dehaze(const Mat& src, int r, double topRatio, double t0, double omega, double eps);

int main(int argc, char *argv[]) {

    //传入参数没有图片信息，退出
    if(argc < 3){
        cout << "参数错误"<<endl;
        return -1;
    }

    const string fileName = argv[1];
    const int type = atoi(argv[2]);

    const int filterRadius = 7;

    //top ratio for the brightest pixels
    const double topRatio = 0.001;

    //lower bound t0
    const double t0 = 0.1;

    //keep more haze for the distant objects
    const double omega = 0.95;

    //regularization parameter
    const double eps = 10E-6;


    if(type == 0){
        Mat src = imread(fileName);

        dehaze(src,filterRadius,topRatio,t0,omega,eps);

        waitKey(0);
    } else{
        VideoCapture videoCapture(fileName);
        if (!videoCapture.isOpened()){
            cout<<"视频打开失败"<<endl;
            return -1;
        }
        long totalFrameNumber = videoCapture.get(CAP_PROP_FRAME_COUNT);
        int frameWidth = videoCapture.get(CAP_PROP_FRAME_WIDTH);
        int frameHeight = videoCapture.get(CAP_PROP_FRAME_HEIGHT);
        double rate = videoCapture.get(CAP_PROP_FPS);

        long frameStart = 0;
        videoCapture.set(CAP_PROP_POS_FRAMES,frameStart);
        long currentFrame = frameStart;

        namedWindow("result");

        while(currentFrame < totalFrameNumber){
            Mat image;
            videoCapture.read(image);
            image = dehaze(image,filterRadius,topRatio,t0,omega,eps);
            currentFrame++;
            imshow("result",image);
            waitKey(1);
        }
        videoCapture.release();
        waitKey(0);
    }

    return 0;
}


void gammaCorrection(Mat& src, Mat& dst, float fGamma)
{
    CV_Assert(src.data);

    // accept only char type matrices
    CV_Assert(src.depth() != sizeof(uchar));

    // build look up table
    unsigned char lut[256];
    for( int i = 0; i < 256; i++ )
    {
        lut[i] = saturate_cast<uchar>(pow((float)(i/255.0), fGamma) * 255.0f);
    }

    dst = src.clone();
    const int channels = dst.channels();
    switch(channels)
    {
        case 1:
        {

            MatIterator_<uchar> it, end;
            for( it = dst.begin<uchar>(), end = dst.end<uchar>(); it != end; it++ )
                //*it = pow((float)(((*it))/255.0), fGamma) * 255.0;
                *it = lut[(*it)];

            break;
        }
        case 3:
        {

            MatIterator_<Vec3b> it, end;
            for( it = dst.begin<Vec3b>(), end = dst.end<Vec3b>(); it != end; it++ )
            {
                //(*it)[0] = pow((float)(((*it)[0])/255.0), fGamma) * 255.0;
                //(*it)[1] = pow((float)(((*it)[1])/255.0), fGamma) * 255.0;
                //(*it)[2] = pow((float)(((*it)[2])/255.0), fGamma) * 255.0;
                (*it)[0] = lut[((*it)[0])];
                (*it)[1] = lut[((*it)[1])];
                (*it)[2] = lut[((*it)[2])];
            }

            break;

        }
    }
}

Mat dehaze(const Mat& src, int r, double topRatio, double t0, double omega, double eps){
    //输出图片大小
    cout<<"图片大小为: "<<src.rows <<"*" << src.cols<<endl;

    //开始计时
    double start =clock();

    //图片必须为彩色图片
    CV_Assert(!src.empty() && src.channels() == 3);

    Vec3b atmosphericLight = EstimationAtmosphericLight(src);
    Vec3f a = atmosphericLight;
    a[0] = (float)(atmosphericLight[0]/255.0);
    a[1] = (float)(atmosphericLight[1]/255.0);
    a[2] = (float)(atmosphericLight[2]/255.0);
//    Mat transmission = EstimationTransmission(src, atmosphericLight,filterRadius, omega, eps);
//    transmission.convertTo(transmission,CV_32FC1,1.0/255.0);
    Mat fSrc;
    src.convertTo(fSrc,CV_32FC3,1.0/255.0);
    //图片归一化
//    if (src.type() != CV_32FC3){
//        src.convertTo(src,CV_32FC3,1.0/255.0);
//    }

    //计算大气光
//    Vec3f atmosphericLight = estimateAtmosphericLight(src,filterRadius,topRatio);

    //计算投射图
    Mat transmission = estimateTransmission(fSrc, a,r, omega, eps);

    //恢复图像
    Mat recoverImage = recover(fSrc,transmission,a,t0);

    recoverImage.convertTo(recoverImage,CV_8UC3,255);

    gammaCorrection(recoverImage,recoverImage,0.7);

    //停止计时
    double stop = clock();

    //输出耗时
    cout<<"恢复图片耗时: "<<(stop-start)/CLOCKS_PER_SEC*1000 <<"ms"<<endl;

//    imshow("source image",src);
//    imshow("transmission map",transmission);
//    imshow("recover image",recoverImage);

    return recoverImage;
}