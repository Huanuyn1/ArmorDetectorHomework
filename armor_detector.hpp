#pragma once

#include <opencv2/core/mat.hpp>
#include <string>

bool detectArmor(const cv::Mat& image,const std::string& color,cv::Mat& outputImage,bool verbose=true);
