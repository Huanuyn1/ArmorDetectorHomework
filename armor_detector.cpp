#include "armor_detector.hpp"
#include <cstddef>
#include <opencv2/geometry/2d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <algorithm>
#include <vector>

bool detectArmor(const cv::Mat& image,const std::string& color,cv::Mat& outputImage,bool verbose)
{
    outputImage=image.clone();

    cv::Mat hsv;
    cv::cvtColor(image,hsv,cv::COLOR_BGR2HSV);

    cv::Mat colormask;
    if(color=="red"){
        cv::Mat lowRedmask;
        cv::inRange(hsv,cv::Scalar(0,170,80),cv::Scalar(10,255,255),lowRedmask);
        cv::Mat highRedmask;
        cv::inRange(hsv,cv::Scalar(165,170,80),cv::Scalar(180,255,255),highRedmask);

        cv::bitwise_or(lowRedmask,highRedmask,colormask);//析取，合并低红区和高红区
    }
    else if(color=="blue"){
        cv::inRange(hsv,cv::Scalar(90,170,80),cv::Scalar(130,255,255),colormask);//提高饱和度下限，减少蓝灰色窗户干扰
    }

    cv::Mat searchArea;
    cv::dilate(colormask,searchArea,cv::Mat(),cv::Point(-1,-1),30);//把红色区域向外扩大一点

    cv::Mat brightMask;
    cv::inRange(hsv,cv::Scalar(0,0,220),cv::Scalar(179,180,255),brightMask);//在扩大的范围里查找白色就是过曝的像素

    cv::Mat WhiteNearmask;
    cv::bitwise_and(searchArea,brightMask,WhiteNearmask);//合取，得到红旁边的白

    cv::Mat result=WhiteNearmask.clone();//这里作为结果，效果比较好

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(
        result,
        contours,
        cv::RETR_EXTERNAL,
        cv::CHAIN_APPROX_SIMPLE
    );

    if(verbose){
        std::cout<<"Contour count:"<<contours.size()<<std::endl;
    }

    std::vector<cv::RotatedRect> lightBars;
    for(std::size_t i=0;i<contours.size();++i){
        cv::drawContours(
            outputImage,
            contours,
            static_cast<int>(i),
            cv::Scalar(255,0,255),
            2
        );
        cv::RotatedRect rect = cv::minAreaRect(contours[i]);//第i个轮廓的所有点，返回面积尽量小可以旋转的外接矩形

        if (rect.size.width==0||rect.size.height==0){
            continue;//排除孤立像素
        }

        float length = std::max(rect.size.width, rect.size.height);
        float width = std::min(rect.size.width, rect.size.height);
        if(verbose){
            std::cout <<"灯条"<<i<<":length="<<length<<",width="<<width<<'\n';//确认长宽
        }

        cv::Point2f vertices[4];
        rect.points(vertices);//获取4个定点

        for (int j=0;j<4;j++){
            cv::line(
                outputImage,
                vertices[j],
                vertices[(j+1)%4],//让最后一个点重新连接第一个点
                cv::Scalar(0,255,0),
                2
            );
        }

        float edge01=cv::norm(vertices[0]-vertices[1]);
        float edge12=cv::norm(vertices[1]-vertices[2]);
        cv::Point2f midpoint1,midpoint2;
        if(edge01<edge12){
            midpoint1=(vertices[0]+vertices[1])*0.5f;
            midpoint2=(vertices[2]+vertices[3])*0.5f;
        }
        else{
            midpoint1=(vertices[1]+vertices[2])*0.5f;
            midpoint2=(vertices[3]+vertices[0])*0.5f;
        }
        //检测长短边
        cv::Point2f top=midpoint1;
        cv::Point2f bottom=midpoint2;

        if(top.y>bottom.y){
            std::swap(top,bottom);
        }
        //分清上下端点
        cv::circle(outputImage,top,1,cv::Scalar(0,0,255),cv::FILLED);
        cv::circle(outputImage,bottom,1,cv::Scalar(0,0,255),cv::FILLED);
        //画。
        lightBars.push_back(rect);
    }

    if(lightBars.size()!=2){
        if(verbose){
            std::cerr<<"不合法的有效灯条数量\n";
        }
        return false;
    }

    cv::Point2f center1=lightBars[0].center;
    cv::Point2f center2=lightBars[1].center;
    cv::Point2f armorCenter=(center1+center2)*0.5f;//找到装甲板中心点

    if(verbose){
        std::cout <<"装甲板中心点:("<< armorCenter.x<<","<<armorCenter.y<<")\n";
    }
    cv::circle(outputImage,armorCenter,4,cv::Scalar(0,200,200),cv::FILLED);
    return true;
}
