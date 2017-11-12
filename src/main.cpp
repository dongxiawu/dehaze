#include <iostream>
#include <regex>
#include "dehaze.h"

using namespace std;
using namespace cv;

int judgeFileType(string fileName);

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
            cout<<"恢复图片耗时: "<< costTime <<"ms"<<endl;

            if (saveFileName.compare("")!=0){
                videoWriter.write(result);
            }

            imshow("result",result);

            //第一帧停顿
            if (currentFrame == frameStart){
                waitKey(0);
            }
            waitKey(max(int(frameInterval - costTime),1));
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