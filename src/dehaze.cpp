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
void DeHaze::gammaInit(float gmama){
    for( int i = 0; i < 256; i++ )
    {
        look_up_table[i] = saturate_cast<uchar>(pow((float)(i/255.0), gmama) * 255.0f);
    }
}

void DeHaze::gammaCorrection(cv::Mat& image){
    CV_Assert(image.data);

    // accept only char type matrices
    CV_Assert(image.depth() != sizeof(uchar));

//    dst = src.clone();
    const int channels = image.channels();
    switch(channels)
    {
        case 1:
        {
            MatIterator_<uchar> it, end;
            for( it = image.begin<uchar>(), end = image.end<uchar>(); it != end; it++ )
                *it = look_up_table[(*it)];
            break;
        }
        case 3:
        {
            MatIterator_<Vec3b> it, end;
            for( it = image.begin<Vec3b>(), end = image.end<Vec3b>(); it != end; it++ )
            {
                (*it)[0] = look_up_table[((*it)[0])];
                (*it)[1] = look_up_table[((*it)[1])];
                (*it)[2] = look_up_table[((*it)[2])];
            }
            break;
        }
    }
}

DeHaze::DeHaze(int r, double t0, double omega, double eps) : r(r), t0(t0), omega(omega), eps(eps)
{
    gammaInit(0.7);
    atmosphericLight = Vec3f(0,0,0);
}

cv::Mat DeHaze::imageHazeRemove(const cv::Mat& I)
{
    CV_Assert(I.channels() == 3);
    if (I.depth() != CV_32F){
        I.convertTo(this->I, CV_32F,1.0/255.0);
    }
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

    cout<<"atmosphericLight" << atmosphericLight << endl;

    return atmosphericLight;
}

cv::Mat DeHaze::estimateTransmission(){
    CV_Assert(I.channels() == 3);
    vector<Mat> channels;
    split(I,channels);

    channels[0] = channels[0]/atmosphericLight[0];
    channels[1] = channels[1]/atmosphericLight[1];
    channels[2] = channels[2]/atmosphericLight[2];

    Mat normalized;
    merge(channels,normalized);

    Mat darkChannel = calcDarkChannel(normalized,r);

    imshow("dark channel",darkChannel);

    transmission = 1.0 - omega * darkChannel;

    //透射率修正
    float k = 0.3;
    transmission = min(max(k/abs(1-darkChannel),1).mul(transmission),1);

    //导向滤波耗时30ms左右
//    transmission = guidedFilter(src, transmission, 8*r, eps);

    //10ms左右
    transmission = fastGuidedFilter(I, transmission, 8*r, 4, eps);

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
    CV_Assert(I.channels() == 3);
    vector<Mat> channels;
    split(I,channels);

    channels[0] = channels[0]/atmosphericLight[0];
    channels[1] = channels[1]/atmosphericLight[1];
    channels[2] = channels[2]/atmosphericLight[2];

    Mat normalized;
    merge(channels,normalized);

    Mat darkChannel = calcDarkChannel(normalized,r);

    transmission = 1.0 - omega * darkChannel;

    //透射率修正
    float k = 0.2;
    transmission = min(max(k/abs(1-darkChannel),1).mul(transmission),1);

    //导向滤波耗时30ms左右
//    transmission = guidedFilter(I, transmission, 8*r, eps);

    //10ms左右
    transmission = fastGuidedFilter(I, transmission, 8*r, 4, eps);


    //截断，这里应该处理  3-5ms
    for (int i = 0; i < transmission.rows; i+=r) {
        for (int j = 0; j < transmission.cols; j+=r) {
            int w = (j+r < transmission.cols) ? r : transmission.cols-j;
            int h = (i+r < transmission.rows) ? r : transmission.rows-i;
            Mat roi(transmission,Rect(j,i,w,h));
            cv::Scalar mean, std, score;
            meanStdDev(roi,mean,std);
            score = mean -std;
//            cout<<score<<endl;
            if (score.val[0] < 0.04){
                int x = j > r? j-r:0;
                int y = i > r? i-r:0;
                int ww = (x+2*r < transmission.cols) ? 2*r : transmission.cols-x;
                int hh = (y+2*r < transmission.rows) ? 2*r : transmission.rows-y;
                Mat filterRoi(transmission,Rect(x,y,ww,hh));
                GaussianBlur(filterRoi,filterRoi,Size(2*r+1,2*r+1),0,0);
            }
        }
    }

    imshow("transmission",transmission);

    return transmission;
}

cv::Mat DeHaze::recover(){
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
    gammaCorrection(recover);//耗时较大 不过还是有必要的 10ms左右 可以考虑别的矫正方法YUV

    return recover;
}