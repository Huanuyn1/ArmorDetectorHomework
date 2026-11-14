#include "armor_detector.hpp"
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/videoio.hpp>
#include <string>
#include <iostream>

int main(int argc,char* argv[])
{
    if(argc!=2){
        std::cerr<<"Usage:"<<argv[0]<<"<red|blue>\n";
        return 1;
    }
    std::string color=argv[1];
    if(color!="red"&&color!="blue"){
        std::cerr<<"Unknown color:"<<color<<'\n';
        return 1;
    }

    cv::VideoCapture capture("videos/Video_20261101162011859.mp4");
    if(!capture.isOpened()){
        std::cerr<<"Cannot open video\n";
        return 1;
    }

    double fps=capture.get(cv::CAP_PROP_FPS);
    int frameWidth=static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
    int frameHeight=static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    if(fps<=0||frameWidth<=0||frameHeight<=0){
        std::cerr<<"Invalid video properties\n";
        return 1;
    }

    cv::VideoWriter writer(
        "test/armor_result.mp4",
        cv::VideoWriter::fourcc('a','v','c','1'),
        fps,
        cv::Size(frameWidth,frameHeight)
    );
    if(!writer.isOpened()){
        std::cerr<<"Cannot open video writer\n";
        return 1;
    }

    int frameIndex=0;
    int detectedCount=0;
    while(true){
        cv::Mat frame;
        capture>>frame;
        if(frame.empty()){
            break;
        }

        cv::Mat outputImage;
        bool detected=detectArmor(frame,color,outputImage,false);
        if(detected){
            ++detectedCount;
        }
        writer.write(outputImage);

        if(frameIndex%100==0){
            std::cout<<"Frame "<<frameIndex<<",detected="<<detected<<'\n';
        }
        ++frameIndex;
    }

    capture.release();
    writer.release();
    std::cout<<"Frame count:"<<frameIndex<<'\n';
    std::cout<<"Detected count:"<<detectedCount<<'\n';
    return 0;
}
