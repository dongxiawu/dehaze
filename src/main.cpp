#include <iostream>
#include <regex>
#include "dehaze.h"

using namespace std;
using namespace cv;

int judgeFileType(string fileName);
//评价图像
void calcEvaluatingIndicator(const Mat& src);

int main(int argc, char *argv[]) {

//    传入参数没有图片信息，退出
    if(argc < 2){
        cout << "参数错误"<<endl;
        return -1;
    }

    const string fileName = argv[1];
    const String saveFileName = argc >=3 ? argv[2] : "";

//    const string fileName = "videos/hazeroad.avi";
//    const String saveFileName = "";


    const int type = judgeFileType(fileName);

    const int filterRadius = 7;

    //lower bound t0
    const double t0 = 0.1;

    //keep more haze for the distant objects
    const double omega = 0.95;

    //regularization parameter
    const double eps = 10E-6;

    if(type == 0){
        Mat src = imread(fileName);
        imshow("source image",src);

        //输出图片大小
        cout<<"图片大小为: "<<src.rows <<"*" << src.cols<<endl;

        //开始计时
        double start =clock();

        DeHaze deHaze(filterRadius,t0,omega,eps);
        Mat recover = deHaze.imageHazeRemove(src);

        calcEvaluatingIndicator(recover);

        //停止计时
        double stop = clock();

        //保存图片
        if (saveFileName.compare("")!=0){
            imwrite(saveFileName,recover);
        }

        //输出耗时
        cout<<"恢复图片耗时: "<<(stop-start)/CLOCKS_PER_SEC*1000 <<"ms"<<endl;

        imshow("recover image",recover);

        waitKey(0);
    } else if(type == 1){
        VideoCapture videoCapture(fileName);

        VideoWriter videoWriter;

        if (!videoCapture.isOpened()){
            cout<<"视频打开失败"<<endl;
            return -1;
        }

        //视频流参数
        long totalFrameNumber = videoCapture.get(CAP_PROP_FRAME_COUNT);
        int frameWidth = videoCapture.get(CAP_PROP_FRAME_WIDTH);
        int frameHeight = videoCapture.get(CAP_PROP_FRAME_HEIGHT);
        double rate = videoCapture.get(CAP_PROP_FPS);

        if(saveFileName.compare("") != 0){
            videoWriter = VideoWriter(saveFileName,CV_FOURCC_DEFAULT,rate,Size(frameWidth,frameHeight));
        }

        double frameInterval = 1000.0/rate;

        cout<<"视频大小： "<< frameHeight <<"*"<< frameWidth<<endl;
        cout<<"帧数： "<< totalFrameNumber <<endl;
        cout<<"帧率： "<< rate <<endl;

        long frameStart = 0;
        videoCapture.set(CAP_PROP_POS_FRAMES,frameStart);
        long currentFrame = frameStart;

        namedWindow("source");
        namedWindow("result");

        DeHaze deHaze(filterRadius, t0, omega, eps);
        deHaze.setFPS(rate);

        while(currentFrame < totalFrameNumber){
            Mat source;
            videoCapture.read(source);
            imshow("source",source);

            //开始计时
            double start =clock();

            Mat result = deHaze.videoHazeRemove(source);

            //停止计时
            double stop = clock();

            double costTime = (stop-start)/CLOCKS_PER_SEC*1000;
            //输出耗时
            cout<<"处理当前帧总耗时: "<< costTime <<"ms"<<endl;

            if (saveFileName.compare("")!=0){
                videoWriter.write(result);
            }

            imshow("result",result);

            //第一帧停顿
            if (currentFrame == frameStart){
                waitKey(0);
            }
            waitKey(max(int(frameInterval - costTime),1));
//            waitKey(0);
//            cout<<"等待时间："<<max(int(frameInterval - costTime),1)<<"ms"<<endl;
            currentFrame++;
        }

        videoCapture.release();

        if (saveFileName.compare("")!=0){
            videoWriter.release();
        }

        waitKey(0);
    }

    return 0;
}

int judgeFileType(string fileName){
    int pos = fileName.find('.');
    string hz = fileName.substr(pos+1);
    transform(hz.begin(),hz.end(),hz.begin(),::tolower);
    regex imagePattern("jpg|png|bmp|jpeg");
    regex videoPattern("avi|mp4|rm|rmvb|3gp|wmv");
    if (regex_match(hz,imagePattern)){
        return 0;
    }
    if (regex_match(hz,videoPattern)){
        return 1;
    }
    return -1;

}

//评价图像
void calcEvaluatingIndicator(const Mat& src){
    CV_Assert(src.type() == CV_8UC3);

    //均值
    Scalar mean;
    //标准差
    Scalar std;
    meanStdDev(src,mean,std);

    cout << "图像均值为：" <<  (mean.val[0] + mean.val[1] + mean.val[2])/3 << endl;
    cout << "图像标准差为：" <<  (std.val[0] + std.val[1] + std.val[2])/3 << endl;

    int border=1;
    //边缘扩大
    Mat borderTemp(src.rows + border*2, src.cols + border*2, src.depth());
    copyMakeBorder(src, borderTemp, border, border,
                   border, border, BORDER_REPLICATE);

    double gradientSum = 0;

    for (int i = 1; i < borderTemp.rows-1; ++i) {
        for (int j = 1; j < borderTemp.cols-1; ++j) {
            Vec3b valueX = borderTemp.at<Vec3b>(i,j-1);
            Vec3b valueY = borderTemp.at<Vec3b>(i-1,j);
            Vec3b value = borderTemp.at<Vec3b>(i,j);
            gradientSum += sqrt((value[0] - valueX[0])*(value[0] - valueX[0])
                                + (value[0] - valueY[0])*(value[0] - valueY[0]))
                           + sqrt((value[1] - valueX[1])*(value[1] - valueX[1])
                                  + (value[1] - valueY[1])*(value[1] - valueY[1]))
                           + sqrt((value[2] - valueX[2])*(value[2] - valueX[2])
                                  + (value[2] - valueY[2])*(value[2] - valueY[2]));
        }
    }

    cout << "图像平均梯度为：" <<  gradientSum/src.cols/src.rows/src.channels() << endl;

    //熵值(平均信息量)
    double result = 0;
    double pixelRate[256]={0};
    Mat gray;

    cvtColor(src,gray,COLOR_BGR2GRAY);

    for (int i = 0; i < gray.rows; ++i) {
        for (int j = 0; j < gray.cols; ++j) {
            pixelRate[gray.at<uchar>(i,j)]++;
        }
    }

    for(int i=0;i<256;i++)
    {
        pixelRate[i] = pixelRate[i]/(src.rows*src.cols);
    }


    // 根据定义计算图像熵
    for(int i =0;i<256;i++)
    {
        if(pixelRate[i]==0.0)
            result = result;
        else
            result = result-pixelRate[i]*(log(pixelRate[i])/log(2.0));
    }

    cout << "图像信息熵为：" <<  result << endl;

}