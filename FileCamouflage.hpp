#ifndef _FILE_CAMOUFLAGE_HPP_
#define _FILE_CAMOUFLAGE_HPP_

#include <fstream>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <string>

namespace zwn {

constexpr size_t K1_WIDTH = 1920;
constexpr size_t K1_HEIGH = 1080;
constexpr size_t K1_SIZE = 2073600;

void pngMatType(const std::string& fp)
{
    cv::Mat img = cv::imread(fp);
    int tp = img.type();
    std::cout << "Type: " << tp << std::endl;
}

}

#endif
