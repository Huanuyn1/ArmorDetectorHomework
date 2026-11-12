#include <cstddef>
#include <opencv2/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

#include <iostream>

int main()
{
    cv::Mat image=cv::imread("imgs/red_straight_ahead.png");
    if(image.empty()){
        std::cout<<"image not found!\n";
        return 1;
    }
    cv::Mat hsv;
    cv::cvtColor(image,hsv,cv::COLOR_BGR2HSV);

    cv::Mat lowRedmask;
    cv::inRange(hsv,cv::Scalar(0,80,80),cv::Scalar(10,255,255),lowRedmask);
    cv::Mat highRedmask;
    cv::inRange(hsv,cv::Scalar(165,80,80),cv::Scalar(180,255,255),highRedmask);

    cv::Mat Redmask;
    cv::bitwise_or(lowRedmask,highRedmask,Redmask);//析取，合并低红区和高红区

    cv::Mat searchArea;
    cv::dilate(Redmask,searchArea,cv::Mat(),cv::Point(-1,-1),30);//把红色区域向外扩大一点

    cv::Mat brightMask;
    cv::inRange(hsv,cv::Scalar(0,0,220),cv::Scalar(179,180,255),brightMask);//在扩大的范围里查找白色就是过曝的像素

    cv::Mat WhiteNearmask;
    cv::bitwise_and(searchArea,brightMask,WhiteNearmask);//合取，得到红旁边的白

    cv::Mat result=WhiteNearmask.clone();

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(
        result,
        contours,
        cv::RETR_EXTERNAL,
        cv::CHAIN_APPROX_SIMPLE
    );

    std::cout<<"Contour count:"<<contours.size()<<std::endl;
    cv::imwrite("test/result.png",result);

    cv::Mat contourImage=image.clone();
    for(std::size_t i=0;i<contours.size();++i){
        cv::drawContours(
            contourImage,
            contours,
            static_cast<int>(i),
            cv::Scalar(255,0,0),
            2
        );
    }
    cv::imwrite("test/contours.png", contourImage);
    return 0;
}
