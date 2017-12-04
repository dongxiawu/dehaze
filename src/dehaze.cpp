//
// Created by dongxia on 17-11-8.
//
#include <guidedfilter.h>
#include "dehaze.h"
#include "darkchannel.h"
#include "fastguidedfilter.h"

using namespace cv;
using namespace std;

void DeHaze::setFPS(int fps){
    this->fps = fps;
}

DeHaze::DeHaze(int r, double t0, double omega, double eps) : r(r), t0(t0), omega(omega), eps(eps)
{
    for( int i = 0; i < 256; i++ )
    {
        look_up_table[i] = saturate_cast<uchar>(pow((float)(i/255.0), 0.7) * 255.0f);
    }
    lookUpTable =Mat(1, 256, CV_8U);
    uchar* p = lookUpTable.ptr();
    for( int i = 0; i < 256; ++i)
        p[i] = look_up_table[i];
}

cv::Mat DeHaze::imageHazeRemove(const cv::Mat& I)
{
    CV_Assert(I.channels() == 3);
    if (I.depth() != CV_32F){
        I.convertTo(this->I, CV_32F,1.0/255.0);
    }

    cvtColor(I,this->I_YUV,COLOR_BGR2YUV);
    this->I_YUV.convertTo(this->I_YUV, CV_32F,1.0/255.0);

    estimateAtmosphericLight();
    estimateTransmission();
    return recover();
}

cv::Mat DeHaze::videoHazeRemove(const cv::Mat& I){
    CV_Assert(I.channels() == 3);

    if (I.depth() != CV_32F){
        I.convertTo(this->I, CV_32F,1.0/255.0);
    }

    estimateAtmosphericLightVideo();
    estimateTransmissionVideo();
    return recover();
}

//3-5ms
cv::Vec3f DeHaze::estimateAtmosphericLight(){

    double start = clock();

    Mat minChannel = calcMinChannel(I);

    double maxValue = 0;
    Mat aimRoi;

    for (int i = 0; i < minChannel.rows; i+=r) {
        for (int j = 0; j < minChannel.cols; j+=r) {
            int w = (j+r < minChannel.cols) ? r : minChannel.cols-j;
            int h = (i+r < minChannel.rows) ? r : minChannel.rows-i;
            Mat roi(minChannel,Rect(j,i,w,h));
            cv::Scalar mean, std, score;
            meanStdDev(roi,mean,std);
            score = mean -std;
            if (score.val[0] > maxValue){
                maxValue = score.val[0];
                aimRoi = Mat(I,Rect(j,i,w,h));
            }
        }
    }

    cv::Scalar mean,std;
    meanStdDev(aimRoi,mean,std);

    atmosphericLight[0] = static_cast<float> (mean.val[0]);
    atmosphericLight[1] = static_cast<float> (mean.val[1]);
    atmosphericLight[2] = static_cast<float> (mean.val[2]);

    double stop = clock();

    cout<<"atmosphericLight" << atmosphericLight << endl;
    cout<<"估计大气光耗时："<<(stop - start)/CLOCKS_PER_SEC*1000<<"ms"<<endl;

    return atmosphericLight;
}

cv::Mat DeHaze::estimateTransmission(){

    double start = clock();

    CV_Assert(I.channels() == 3);
    vector<Mat> channels;
    split(I_YUV,channels);

    channels[0] = channels[0]/(atmosphericLight[0]*0.114 + atmosphericLight[1]*0.587 + atmosphericLight[2]*0.299);
//    channels[1] = channels[1]/atmosphericLight[1];
//    channels[2] = channels[2]/atmosphericLight[2];

//    Mat normalized;
//    merge(channels,normalized);

//    Mat darkChannel = calcDarkChannel(normalized,r);
    Mat darkChannel = calcDarkChannel(channels[0],r);

//    imshow("dark channel",darkChannel);

    transmission = 1 - omega * darkChannel;

    //导向滤波耗时30ms左右
//    transmission = guidedFilter(src, transmission, 8*r, eps);

    //透射率修正
    float k = 0.3;
    transmission = min(max(k/abs(1-darkChannel),1).mul(transmission),1);

//    Mat gray;
//    cvtColor(I,gray,CV_BGR2GRAY);
    //10ms左右
    transmission = fastGuidedFilter(channels[0], transmission, 8*r, 4, eps);

    double stop = clock();

    cout<<"估计透射率耗时："<<(stop - start)/CLOCKS_PER_SEC*1000<<"ms"<<endl;

//    imshow("transmission",transmission);

    return transmission;
}

cv::Vec3f DeHaze::estimateAtmosphericLightVideo(){

    atmosphericLight = estimateAtmosphericLight();

    while (atmosphericLightQueue.size() < fps*2){
        atmosphericLightQueue.push(atmosphericLight);
        atmosphericLightSum += atmosphericLight;
    }

    atmosphericLight = atmosphericLightSum/(fps*2);

    atmosphericLightSum -= atmosphericLightQueue.front();
    atmosphericLightQueue.pop();

    return atmosphericLight;
}

cv::Mat DeHaze::estimateTransmissionVideo(){
    if (preI.empty()){
        preI = this->I.clone();
        estimateTransmission();
    } else{
        Scalar mean,std;
//        Mat diff = abs(I-preI);
        Mat diff;
        absdiff(I,preI,diff);
        cv::meanStdDev(diff,mean,std);
//        cout << std.val[0]+std.val[1]+std.val[2] << endl;
        if(std.val[0]+std.val[1]+std.val[2] >0.01){
            estimateTransmission();
            cout<<"estimateTransmission"<<endl;
        }
        preI = this->I.clone();
        cout<<"std total:"<<std.val[0]+std.val[1]+std.val[2]<<endl;
        imshow("diff",diff);
    }

//    Mat I_temp,preI_temp;
//    I.convertTo(I_temp,CV_8UC3,255);
//    preI.convertTo(preI_temp,CV_8UC3,255);
//    cout << "different:"<< getPSNR(I,preI) << endl;


//    Scalar diff = getMSSIM(I,preI);
//    cout << "different:"<< diff*100 << endl;
//    estimateTransmission();

    return transmission;

}

cv::Mat DeHaze::recover(){

    double start = clock();

    CV_Assert(I.channels() == 3 && I.depth() == CV_32F);
    vector<Mat> channels;
    split(I,channels);

//	Mat t = max(t0,transmission);
    Mat t = transmission;

    channels[0] = (channels[0]-atmosphericLight[0])/t + atmosphericLight[0];
    channels[1] = (channels[1]-atmosphericLight[1])/t + atmosphericLight[1];
    channels[2] = (channels[2]-atmosphericLight[2])/t + atmosphericLight[2];

    Mat recover;
    merge(channels,recover);

    recover.convertTo(recover,CV_8UC3,255);

    //可能可以去掉
    LUT(recover,lookUpTable,recover);

    double stop = clock();

    cout<<"恢复图像耗时："<<(stop - start)/CLOCKS_PER_SEC*1000<<"ms"<<endl;

    return recover;
}