//
// Created by dongxia on 17-11-8.
//
#include <guidedfilter.h>
#include "dehaze.h"
#include "darkchannel.h"
#include "fastguidedfilter.h"

using namespace cv;
using namespace std;

double getPSNR(const Mat& I1, const Mat& I2)
{
    Mat s1;
    absdiff(I1, I2, s1);       // |I1 - I2|
    s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
    s1 = s1.mul(s1);           // |I1 - I2|^2

    Scalar s = sum(s1);         // sum elements per channel

    double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels

    if( sse <= 1e-10) // for small values return zero
        return 0;
    else
    {
        double  mse =sse /(double)(I1.channels() * I1.total());
        double psnr = 10.0*log10((255*255)/mse);
        return psnr;
    }
}

Scalar getMSSIM( const Mat& i1, const Mat& i2)
{
    const double C1 = 6.5025, C2 = 58.5225;
    /***************************** INITS **********************************/
    int d     = CV_32F;

    Mat I1, I2;
    i1.convertTo(I1, d);           // cannot calculate on one byte large values
    i2.convertTo(I2, d);

    Mat I2_2   = I2.mul(I2);        // I2^2
    Mat I1_2   = I1.mul(I1);        // I1^2
    Mat I1_I2  = I1.mul(I2);        // I1 * I2

    /*************************** END INITS **********************************/

    Mat mu1, mu2;   // PRELIMINARY COMPUTING
    GaussianBlur(I1, mu1, Size(11, 11), 1.5);
    GaussianBlur(I2, mu2, Size(11, 11), 1.5);

    Mat mu1_2   =   mu1.mul(mu1);
    Mat mu2_2   =   mu2.mul(mu2);
    Mat mu1_mu2 =   mu1.mul(mu2);

    Mat sigma1_2, sigma2_2, sigma12;

    GaussianBlur(I1_2, sigma1_2, Size(11, 11), 1.5);
    sigma1_2 -= mu1_2;

    GaussianBlur(I2_2, sigma2_2, Size(11, 11), 1.5);
    sigma2_2 -= mu2_2;

    GaussianBlur(I1_I2, sigma12, Size(11, 11), 1.5);
    sigma12 -= mu1_mu2;

    ///////////////////////////////// FORMULA ////////////////////////////////
    Mat t1, t2, t3;

    t1 = 2 * mu1_mu2 + C1;
    t2 = 2 * sigma12 + C2;
    t3 = t1.mul(t2);              // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))

    t1 = mu1_2 + mu2_2 + C1;
    t2 = sigma1_2 + sigma2_2 + C2;
    t1 = t1.mul(t2);               // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))

    Mat ssim_map;
    divide(t3, t1, ssim_map);      // ssim_map =  t3./t1;

    Scalar mssim = mean( ssim_map ); // mssim = average of ssim map
    return mssim;
}

void DeHaze::setFPS(int fps){
    this->fps = fps;
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
    split(I,channels);

    channels[0] = channels[0]/atmosphericLight[0];
    channels[1] = channels[1]/atmosphericLight[1];
    channels[2] = channels[2]/atmosphericLight[2];

    Mat normalized;
    merge(channels,normalized);

    Mat darkChannel = calcDarkChannel(normalized,r);

//    imshow("dark channel",darkChannel);

    transmission = 1.0 - omega * darkChannel;

    //导向滤波耗时30ms左右
//    transmission = guidedFilter(src, transmission, 8*r, eps);

    //透射率修正
    float k = 0.3;
    transmission = min(max(k/abs(1-darkChannel),1).mul(transmission),1);

    Mat gray;
    cvtColor(I,gray,CV_BGR2GRAY);
    //10ms左右
    transmission = fastGuidedFilter(gray, transmission, 8*r, 4, eps);

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

    pow(recover,0.7,recover);//3ms gamma矫正
    recover.convertTo(recover,CV_8UC3,255);

    double stop = clock();

    cout<<"恢复图像耗时："<<(stop - start)/CLOCKS_PER_SEC*1000<<"ms"<<endl;

    return recover;
}