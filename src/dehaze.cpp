//
// Created by dongxia on 17-11-8.
//
#include "dehaze.h"
#include "darkchannel.h"
#include "fastguidedfilter.h"

using namespace cv;
using namespace std;

static void gammaCorrection(Mat& src, Mat& dst, float fGamma)
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

DeHaze::DeHaze(int r, double t0, double omega, double eps) : r(r), t0(t0), omega(omega), eps(eps)
{

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

cv::Vec3f DeHaze::estimateAtmosphericLight(){
//    Vec3b atmosphericLight(0,0,0);

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

    atmosphericLight[0] = mean.val[0];
    atmosphericLight[1] = mean.val[1];
    atmosphericLight[2] = mean.val[2];

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
    transmission = 1.0 - omega * darkChannel;

    //透射率修正
    float k = 0.3;
    transmission = min(max(k/abs(1-darkChannel),1).mul(transmission),1);

    //导向滤波耗时30ms左右
//    transmission = guidedFilter(src, transmission, 8*r, eps);

    transmission = fastGuidedFilter(I, transmission, 8*r, 4, eps);

//    transmission = guidedFilter(src,transmission,8*r,eps);

    imshow("transmission",transmission);

    return transmission;
}

cv::Vec3f DeHaze::estimateAtmosphericLightVideo(){

    atmosphericLight = estimateAtmosphericLight();

    while (atmosphericLightVector.size() < 15){
        atmosphericLightVector.push_back(atmosphericLight);
    }

    Vec3f sum(0,0,0);

    for (int i =0;i<atmosphericLightVector.size();i++) {
        sum += atmosphericLightVector[i];
    }

    atmosphericLightVector.erase(atmosphericLightVector.begin());

    atmosphericLight = sum/15;
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
    float k = 0.3;
    transmission = min(max(k/abs(1-darkChannel),1).mul(transmission),1);

    //导向滤波耗时30ms左右
//    transmission = guidedFilter(src, transmission, 8*r, eps);

    transmission = fastGuidedFilter(I, transmission, 8*r, 4, eps);

//    transmission = guidedFilter(src,transmission,8*r,eps);

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

    gammaCorrection(recover,recover,0.7);//耗时较大

    return recover;
}