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

