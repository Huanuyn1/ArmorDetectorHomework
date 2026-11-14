#include "armor_detector.hpp"
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <string>
#include <iostream>

int main(int argc,char* argv[])
{
    if(argc!=2){
        std::cerr<<"Usage:"<<argv[0]<<"<red|blue>\n";
        return 1;
    }
    std::string color=argv[1];
    cv::Mat image=cv::imread("imgs/blue_tilted_left.png");
    if(image.empty()){
        std::cerr<<"image not found!\n";
        return 1;
    }

    cv::Mat outputImage;
    if(!detectArmor(image,color,outputImage)){
        return 1;
    }

    cv::imwrite("test/b_final_l.png",outputImage);
    return 0;
}
