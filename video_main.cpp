#include "armor_detector.hpp"
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/videoio.hpp>
#include <string>
#include <iostream>

int main(int argc,char* argv[])
{
    if(argc<2||argc>3){
        std::cerr<<"Usage: "<<argv[0]<<" <red|blue> [video_path]\n";
        return 1;
    }
    std::string color=argv[1];
    if(color!="red"&&color!="blue"){
        std::cerr<<"Unknown color:"<<color<<'\n';
        return 1;
    }
    const std::string videoPath=argc==3?argv[2]:"../videos/"+color+(color=="red"?".mp4":".avi");

    cv::VideoCapture capture(videoPath,cv::CAP_ANY);
    if(!capture.isOpened()){
        //AVI只是容器，还需要系统具有对应视频编码的解码器；这个错误也可能是路径不正确。
        std::cerr<<"Cannot open video: "<<videoPath<<'\n'
                 <<"Check the path and codec with: ffprobe "<<videoPath<<'\n';
        return 1;
    }

    double fps=capture.get(cv::CAP_PROP_FPS);
    int frameWidth=static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
    int frameHeight=static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    if(fps<=0||frameWidth<=0||frameHeight<=0){
        std::cerr<<"Invalid video properties\n";
        return 1;
    }

    const std::string outputPath="../test/armor_result_"+color+".mp4";
    cv::VideoWriter writer(
        outputPath,
        cv::VideoWriter::fourcc('a','v','c','1'),
        fps,
        cv::Size(frameWidth,frameHeight)
    );
    if(!writer.isOpened()){
        std::cerr<<"Cannot open video writer: "<<outputPath<<'\n';
        return 1;
    }

    int frameIndex=0;
    int detectedCount=0;
    int missedCount=0;
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
        else{
            ++missedCount;
        }
        writer.write(outputImage);

        if(frameIndex%100==0){
            std::cout<<"第 "<<frameIndex<<" 帧："
                     <<(detected?"检测到装甲板":"未检测到装甲板")
                     <<"，累计检出 "<<detectedCount<<" 帧，未检出 "<<missedCount<<" 帧\n";
        }
        ++frameIndex;
    }

    capture.release();
    writer.release();
    std::cout<<"总帧数："<<frameIndex<<'\n';
    std::cout<<"检测成功帧数："<<detectedCount<<'\n';
    std::cout<<"未检出帧数："<<missedCount<<'\n';
    std::cout<<"输出："<<outputPath<<'\n';
    if(frameIndex==0){
        std::cerr<<"No frames were decoded. The AVI codec may not be supported by this OpenCV build.\n";
        return 1;
    }
    return 0;
}
