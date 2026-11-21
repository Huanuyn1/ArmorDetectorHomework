#pragma once

#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <string>
#include <vector>

//评测接口：每块装甲板依次返回左上、左下、右下、右上四个灯条端点。
std::vector<std::vector<cv::Point2i>> detectArmorEndpoints(const cv::Mat& image,const std::string& color);

//可视化入口保留原有返回值和参数，调用方式不变。
bool detectArmor(const cv::Mat& image,const std::string& color,cv::Mat& outputImage,bool verbose=true);
