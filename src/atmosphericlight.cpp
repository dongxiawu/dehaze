#include "atmosphericlight.h"
#include "darkchannel.h"

using namespace cv;
using namespace std;

cv::Vec3f estimateAtmosphericLight(const Mat& src, int r, double topRatio){
    CV_Assert(src.channels() == 3);

    Mat darkChannel = calcDarkChannel(src,r);

    Vec3f atmosphericLight = Vec3f(0,0,0);

    Mat darkChannelVector = darkChannel.reshape(1,1);
    Mat maskIdx;
    sortIdx(darkChannelVector,maskIdx,SORT_EVERY_ROW + SORT_DESCENDING);
    //是不是可以改进速度
    Mat mask = Mat::zeros(maskIdx.rows,maskIdx.cols,maskIdx.type());
    for (int i = 0; i < maskIdx.cols; ++i) {
        mask.at<int>(maskIdx.at<int>(i))=i;
    }

    //top num
    int topNum = (int)(darkChannel.cols * darkChannel.rows * topRatio);

    //CV_32S 类型不能用 threshold 二值化 所以先转换类型 再阈值化
    mask.convertTo(mask,CV_32F);
    threshold(mask,mask,topNum,1,THRESH_BINARY_INV);
    mask.convertTo(mask,CV_8U,255);
    mask = mask.reshape(1,darkChannel.rows);

    vector<Mat> channels;
    split(src,channels);
    Mat total = channels[0] + channels[1] + channels[2];
    Point pos;
    minMaxLoc(total,NULL,NULL,NULL,&pos,mask);

    atmosphericLight[0] = channels[0].at<float>(pos);
    atmosphericLight[1] = channels[1].at<float>(pos);
    atmosphericLight[2] = channels[2].at<float>(pos);

    return atmosphericLight;
}

cv::Vec3b EstimationAtmosphericLight(const cv::Mat& src){
    CV_Assert(src.channels() == 3);

    cv::Vec3b atmosphericLight(0,0,0);


    cv::Mat roi(src,cv::Rect(0,0,src.cols,src.rows));
    int width = roi.cols;
    int height = roi.rows;

    // compare to threshold(200) --> bigger than threshold, divide the block
    while (width * height >200){

        cv::Mat upperLeft(roi,cv::Rect(0, 0, width/2, height/2));
        cv::Mat upperRight(roi,cv::Rect(width/2+width%2, 0, width/2, height/2));
        cv::Mat lowerLeft(roi,cv::Rect(0,height/2+height%2,width/2,height/2));
        cv::Mat lowerRight(roi,cv::Rect(width/2+width%2,height/2+height%2,width/2,height/2));

        double maxScore = 0,curScore;
        int maxIndex =0;

        cv::Scalar mean, std, score;

        cv::meanStdDev(upperLeft,mean,std);
        score = mean - std;
        curScore = (score[0] + score[1] + score[2]);

        if (maxScore < curScore){
            maxScore = curScore;
            maxIndex = 0;
        }

        cv::meanStdDev(upperRight,mean,std);
        score = mean - std;
        curScore = (score[0] + score[1] + score[2]);

        if (maxScore < curScore){
            maxScore = curScore;
            maxIndex = 1;
        }

        cv::meanStdDev(lowerLeft,mean,std);
        score = mean - std;
        curScore = (score[0] + score[1] + score[2]);

        if (maxScore < curScore){
            maxScore = curScore;
            maxIndex = 2;
        }

        cv::meanStdDev(lowerRight,mean,std);
        score = mean - std;
        curScore = (score[0] + score[1] + score[2]);

        if (maxScore < curScore){
            maxScore = curScore;
            maxIndex = 3;
        }
        switch (maxIndex){
            case 0:
                roi = upperLeft;
                break;
            case 1:
                roi = upperRight;
                break;
            case 2:
                roi = lowerLeft;
                break;
            case 3:
                roi = lowerRight;
                break;
        }
        width = roi.cols;
        height = roi.rows;
    }

    int minDistance = 65536;
    int distance;
    for(int nY = 0; nY < height; nY++)
    {
        for(int nX = 0; nX < width; nX++)
        {
            cv::Vec3b p = roi.at<cv::Vec3b>(nY,nX);

            // 255-r, 255-g, 255-b
            distance = int(sqrt((255-p[0])*(255-p[0]) + (255-p[1])*(255-p[1]) + (255-p[2])*(255-p[2])));
            if (minDistance > distance){
                minDistance = distance;
                atmosphericLight = p;
            }
        }
    }
    return atmosphericLight;
}

cv::Vec3f MyEstimateAtmosphericLight(const Mat& src, int r){
    CV_Assert(src.channels() == 3);

    Vec3f atmosphericLight(0,0,0);

    Mat minChannel = calcMinChannel(src);

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
                aimRoi = Mat(src,Rect(j,i,w,h));
            }
        }
    }

    cv::Scalar mean,std;
    meanStdDev(aimRoi,mean,std);

    atmosphericLight[0] = mean.val[0];
    atmosphericLight[1] = mean.val[1];
    atmosphericLight[2] = mean.val[2];

    return atmosphericLight;
}

