#include "armor_detector.hpp"
#include <cstddef>
#include <opencv2/core/version.hpp>
#if CV_VERSION_MAJOR >= 5
#include <opencv2/geometry/2d.hpp>
#endif
#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <limits>

namespace {
struct LightBar {
    cv::RotatedRect rect;
    cv::Point2f top;
    cv::Point2f bottom;
    float length;
    float angle;
};

float verticalAngle(const cv::Point2f& top,const cv::Point2f& bottom)
{
    //top->bottom作为灯条的方向向量，atan2求它和竖直方向的夹角。
    const cv::Point2f direction=bottom-top;
    return std::abs(std::atan2(direction.x,direction.y)*180.0f/static_cast<float>(CV_PI));
}

void drawLightBar(cv::Mat& image,const LightBar& light)
{
    cv::Point2f vertices[4];
    light.rect.points(vertices);
    for(int i=0;i<4;++i){
        cv::line(image,vertices[i],vertices[(i+1)%4],cv::Scalar(0,255,0),2);
    }
    cv::circle(image,light.top,1,cv::Scalar(0,0,255),cv::FILLED);
    cv::circle(image,light.bottom,1,cv::Scalar(0,0,255),cv::FILLED);
}
}

namespace {
std::vector<std::vector<cv::Point2i>> detectArmorImpl(const cv::Mat& image,const std::string& color,
                                                      cv::Mat& outputImage,bool verbose)
{
    outputImage=image.clone();

    if(image.empty()){
        return {};
    }

    if(color!="red"&&color!="blue"){
        if(verbose){
            std::cerr<<"Unsupported color:"<<color<<'\n';
        }
        return {};
    }

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

    //几何筛选单独放在一个代码块里，前面是原来的曝光筛选。
    {
        std::vector<LightBar> lightBars;
        for(std::size_t i=0;i<contours.size();++i){
            //第一步：先单独判断每个轮廓，不急于凑够两个。
            const double contourArea=cv::contourArea(contours[i]);
            if(contourArea<4.0){
                continue;//面积太小的噪点无法构成灯条
            }

            cv::RotatedRect rect=cv::minAreaRect(contours[i]);
            float length=std::max(rect.size.width,rect.size.height);
            float width=std::min(rect.size.width,rect.size.height);
            if(length<8.0f||width<1.5f){
                continue;//远处灯条仍需保留数个像素，过滤孤立噪点
            }

            cv::Point2f vertices[4];
            rect.points(vertices);
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
            //取两条短边的中点作为灯条端点，再按y坐标分清上下。
            cv::Point2f top=midpoint1;
            cv::Point2f bottom=midpoint2;
            if(top.y>bottom.y){
                std::swap(top,bottom);
            }

            LightBar light{rect,top,bottom,length,verticalAngle(top,bottom)};
            //填充率=轮廓面积/外接矩形面积，灯条接近实心矩形时应接近1。
            float fillRatio=static_cast<float>(contourArea/rect.size.area());
            if(light.angle>35.0f||length/width<2.0f||fillRatio<0.45f){
                continue;//灯条应细长、接近竖直且轮廓较饱满
            }
            if(verbose){
                std::cout<<"候选灯条"<<i<<":length="<<length<<",width="<<width
                         <<",angle="<<light.angle<<",fillRatio="<<fillRatio<<'\n';
            }
            lightBars.push_back(light);
            drawLightBar(outputImage,light);
        }

        int bestLeft=-1;
        int bestRight=-1;
        float bestScore=std::numeric_limits<float>::max();
        //第二步：两两组合候选灯条，j=i+1可以避免同一对重复检查。
        for(std::size_t i=0;i<lightBars.size();++i){
            for(std::size_t j=i+1;j<lightBars.size();++j){
                const LightBar& first=lightBars[i];
                const LightBar& second=lightBars[j];
                //findContours不保证从左到右返回轮廓，所以这里按中心x重新分左右。
                const LightBar& left=first.rect.center.x<second.rect.center.x?first:second;
                const LightBar& right=first.rect.center.x<second.rect.center.x?second:first;

                //两根灯条的长度应该接近，短边/长边在0~1之间。
                float lengthRatio=std::min(left.length,right.length)/std::max(left.length,right.length);
                float angleDifference=std::abs(left.angle-right.angle);

                //用中心距离除以平均长度，远处灯条整体变小时也能使用同一阈值。
                cv::Point2f centerDelta=right.rect.center-left.rect.center;
                float centerDistance=cv::norm(centerDelta);
                float averageLength=(left.length+right.length)*0.5f;
                float distanceRatio=centerDistance/averageLength;
                //中心连线应该接近水平，atan2(dy,dx)求它和水平方向的夹角。
                float horizontalAngle=std::abs(std::atan2(centerDelta.y,centerDelta.x)*180.0f/static_cast<float>(CV_PI));

                bool matched=lengthRatio>=0.55f&&angleDifference<=15.0f&&
                             distanceRatio>=0.8f&&distanceRatio<=4.0f&&
                             horizontalAngle<=30.0f;
                if(!matched){
                    continue;
                }

                //多对候选都合法时，选择长度、角度、间距关系最一致的一对。
                float score=std::abs(distanceRatio-2.0f)+1.0f-lengthRatio+
                            angleDifference/15.0f+horizontalAngle/30.0f;
                if(score<bestScore){
                    bestScore=score;
                    bestLeft=static_cast<int>(&left-lightBars.data());
                    bestRight=static_cast<int>(&right-lightBars.data());
                }
            }
        }

        if(bestLeft<0){
            if(verbose){
                std::cerr<<"没有找到满足几何约束的灯条对\n";
            }
            return {};
        }

        const LightBar& left=lightBars[bestLeft];
        const LightBar& right=lightBars[bestRight];
        cv::Point2f armorCenter=(left.rect.center+right.rect.center)*0.5f;
        if(verbose){
            std::cout<<"装甲板中心点:("<<armorCenter.x<<","<<armorCenter.y<<")\n";
        }
        cv::line(outputImage,left.top,right.top,cv::Scalar(0,255,255),2);
        cv::line(outputImage,left.bottom,right.bottom,cv::Scalar(0,255,255),2);
        cv::circle(outputImage,armorCenter,4,cv::Scalar(0,200,200),cv::FILLED);

        //评测要求每块装甲板按左上、左下、右下、右上的顺序返回四个整数端点。
        cv::Point2i leftTop(cvRound(left.top.x),cvRound(left.top.y));
        cv::Point2i leftBottom(cvRound(left.bottom.x),cvRound(left.bottom.y));
        cv::Point2i rightTop(cvRound(right.top.x),cvRound(right.top.y));
        cv::Point2i rightBottom(cvRound(right.bottom.x),cvRound(right.bottom.y));
        return {{leftTop,leftBottom,rightBottom,rightTop}};
    }
}
}

std::vector<std::vector<cv::Point2i>> detectArmorEndpoints(const cv::Mat& image,const std::string& color)
{
    cv::Mat unusedImage;
    return detectArmorImpl(image,color,unusedImage,false);
}

bool detectArmor(const cv::Mat& image,const std::string& color,cv::Mat& outputImage,bool verbose)
{
    return !detectArmorImpl(image,color,outputImage,verbose).empty();
}