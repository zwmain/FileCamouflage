#ifndef _FILE_CAMOUFLAGE_HPP_
#define _FILE_CAMOUFLAGE_HPP_

#include <cstring>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <string>
#include <vector>

namespace zwn {

namespace fs = std::filesystem;

constexpr size_t K1_WIDTH = 1920;
constexpr size_t K1_HEIGH = 1080;
constexpr size_t K1_SIZE = 6220800;
constexpr size_t K1_BUFF = 6220792;
const std::vector<int> PNG_CFG = { cv::IMWRITE_PNG_COMPRESSION, 0, cv::IMWRITE_PNG_STRATEGY, cv::IMWRITE_PNG_STRATEGY_DEFAULT };

void pngMatType(const std::string& fp)
{
    cv::Mat img = cv::imread(fp);
    int tp = img.type();
    std::cout << "Type: " << tp << std::endl;
}

void convertFiltToImage(const std::string& inputFile, const std::string& outputDir)
{
    fs::path fp(inputFile);
    if (!fs::exists(fp)) {
        return;
    }
    if (!fs::is_regular_file(fp)) {
        return;
    }
    fs::path dir(outputDir);
    if (!fs::exists(dir)) {
        bool isOk = fs::create_directory(dir);
        if (!isOk) {
            return;
        }
    } else {
        if (!fs::is_directory(dir)) {
            return;
        }
    }

    uint64_t fileSize = fs::file_size(fp);
    size_t fileCnt = fileSize / K1_BUFF;
    if (fileSize % K1_BUFF != 0) {
        fileCnt += 1;
    }

    std::ifstream fi(fp);
    if (!fi.is_open()) {
        return;
    }
    std::string fileName = fp.filename().string();
    cv::Mat img = cv::Mat::zeros(K1_HEIGH, K1_WIDTH, CV_8UC3);
    uint64_t remainSize = fileSize;
    std::unique_ptr<char[]> buf = std::make_unique<char[]>(K1_BUFF);
    for (size_t i = 0; i < fileCnt; ++i) {
        uchar* data = img.data;
        uint64_t wrSize = remainSize > K1_BUFF ? K1_BUFF : remainSize;
        std::memcpy(data, &wrSize, sizeof(wrSize));
        fi.read(buf.get(), wrSize);
        std::memcpy(&data[8], buf.get(), wrSize);
        remainSize -= wrSize;
        std::string outName = fmt::format("{}_{}.png", fileName, i);
        fs::path outPath = dir / outName;
        cv::imwrite(outPath.string(), img, PNG_CFG);
    }
}

}

#endif
