#include <fastguidedfilter.h>
#include "transmission.h"
#include "darkchannel.h"
#include "guidedfilter.h"

using namespace cv;
using namespace std;

cv::Mat estimateTransmission(const Mat& src, Vec3f atmosphericLight,int r, double omega, double eps){
    CV_Assert(src.channels() == 3);
    vector<Mat> channels;
    split(src,channels);

    channels[0] = channels[0]/atmosphericLight[0];
    channels[1] = channels[1]/atmosphericLight[1];
    channels[2] = channels[2]/atmosphericLight[2];

    Mat normalized;
    merge(channels,normalized);

    Mat darkChannel = calcDarkChannel(normalized,r);
    imshow("dark channel",darkChannel);
    Mat transmission = 1.0 - omega * darkChannel;

    float k = 0.3;
    transmission = min(max(k/abs(1-darkChannel),1).mul(transmission),1);
//    double minValue;
//    minMaxLoc(transmission,&minValue);
//
//    transmission = transmission+0.01-minValue;
//    transmission = min(transmission,1);

    //导向滤波耗时30ms左右
//    transmission = guidedFilter(src, transmission, 8*r, eps);

    transmission = fastGuidedFilter(src, transmission, 8*r, eps);
    imshow("transmission",transmission);

    Mat th;
    threshold(darkChannel,th,0.6,1,THRESH_BINARY_INV);
    imshow("th",th);
    return transmission;
}

float EstimationTransmissionBlock(const cv::Mat& block, cv::Vec3b atmosphericLight){
    CV_Assert(block.channels() == 3 && block.depth() == CV_8U);

    int nCounter;

    int nOutR, nOutG, nOutB;
    int nSquaredOut;
    int nSumofOuts;
    int nSumofSquaredOuts;
    float fTrans, fOptTrs;
    int nTrans;
    int nSumofSLoss;
    float fCost, fMinCost, fMean;
    int nNumberofPixels, nLossCount;


    nNumberofPixels = block.cols * block.rows * 3;

    fTrans = 0.3f;
    nTrans = 427;

    //训练7次
    for(nCounter=0; nCounter<7; nCounter++)
    {
        nSumofSLoss = 0;
        nLossCount = 0;
        nSumofSquaredOuts = 0;
        nSumofOuts = 0;

        for(int y=0; y<block.rows; y++)
        {
            for(int x=0; x<block.cols; x++)
            {
                cv::Vec3b pixel = block.at<cv::Vec3b>(y,x);
                // (I-A)/t + A --> ((I-A)*k*128 + A*128)/128
                nOutB = ((pixel[0] - atmosphericLight[0])*nTrans + 128*atmosphericLight[0])>>7;
                nOutG = ((pixel[1] - atmosphericLight[1])*nTrans + 128*atmosphericLight[1])>>7;
                nOutR = ((pixel[2] - atmosphericLight[2])*nTrans + 128*atmosphericLight[2])>>7;

                if(nOutR>255)
                {
                    nSumofSLoss += (nOutR - 255)*(nOutR - 255);
                    nLossCount++;
                }
                else if(nOutR < 0)
                {
                    nSumofSLoss += nOutR * nOutR;
                    nLossCount++;
                }
                if(nOutG>255)
                {
                    nSumofSLoss += (nOutG - 255)*(nOutG - 255);
                    nLossCount++;
                }
                else if(nOutG < 0)
                {
                    nSumofSLoss += nOutG * nOutG;
                    nLossCount++;
                }
                if(nOutB>255)
                {
                    nSumofSLoss += (nOutB - 255)*(nOutB - 255);
                    nLossCount++;
                }
                else if(nOutB < 0)
                {
                    nSumofSLoss += nOutB * nOutB;
                    nLossCount++;
                }
                nSumofSquaredOuts += nOutB * nOutB + nOutR * nOutR + nOutG * nOutG;;
                nSumofOuts += nOutR + nOutG + nOutB;
            }
        }
        fMean = (float)(nSumofOuts)/(float)(nNumberofPixels);
        float m_fLambda1 = 5.0f;
        fCost = m_fLambda1 * (float)nSumofSLoss/(float)(nNumberofPixels)
                - ((float)nSumofSquaredOuts/(float)nNumberofPixels - fMean*fMean);

        if(nCounter==0 || fMinCost > fCost)
        {
            fMinCost = fCost;
            fOptTrs = fTrans;
        }

        fTrans += 0.1f;
        nTrans = (int)(1.0f/fTrans*128.0f);
    }
    return fOptTrs;

}

cv::Mat EstimationTransmission(const cv::Mat& src, cv::Vec3b atmosphericLight, int r, double omega, double eps){
    CV_Assert(src.channels() == 3);
    int width = src.cols;
    int height = src.rows;

    cv::Mat transmission(src.size(),CV_32FC1);

    for (int i = 0; i < width; i+=r) {
        for (int j = 0; j < height; j+=r) {
            int w = i+r < width ? r : width-i;
            int h = j+r < height ? r : height-j;
            cv::Mat roi(src,cv::Rect(i,j,w,h));
            //TODO
            transmission(cv::Rect(i,j,w,h)) = EstimationTransmissionBlock(roi,atmosphericLight);
        }
    }
    transmission = guidedFilter(src,transmission,r,eps);

    return transmission;
}



