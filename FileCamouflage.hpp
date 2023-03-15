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

enum class Status {
    OK = 0,
    NOT_FILE,
    FILE_NOT_EXISTS,
    FILE_OPEN_ERR,
    FILE_ALREADY_EXISTS,
    NOT_DIR,
    DIR_NOT_EXISTS,
    DIR_CREATE_ERR
};

struct ImageSize {
    size_t width = 0;
    size_t heigh = 0;
    size_t bytes = 0;
    size_t buffs = 0;
};

constexpr size_t K1_WIDTH = 1920;
constexpr size_t K1_HEIGH = 1080;
constexpr size_t K1_SIZE = 6220800;
constexpr size_t K1_BUFF = 6220792;
const std::vector<ImageSize> IMG_LIST {
    { 7860, 4320, 101865600, 101865592 }, // 8k
    { 3840, 2160, 24883200, 24883192 }, // 4k
    { 2560, 1440, 11059200, 11059192 }, // 2k
    { 1920, 1080, 6220800, 6220792 } // 1k
};
const std::vector<int> PNG_CFG { cv::IMWRITE_PNG_COMPRESSION, 0, cv::IMWRITE_PNG_STRATEGY, cv::IMWRITE_PNG_STRATEGY_DEFAULT };

/**
 * @brief 伪装文件
 *
 * @param inputFile 待转换的文件
 * @param outputDir 输出文件夹
 * @return Status 函数执行状态
 */
Status disguiseFile(const std::string& inputFile, const std::string& outputDir);

/**
 * @brief 恢复被伪装的图片
 *
 * @param inputDir 放有图片的文件夹
 * @param outputFile 输出文件路径
 * @return Status 函数执行状态
 */
Status recoveryFile(const std::string& inputDir, const std::string& outputFile);

Status disguiseFile(const std::string& inputFile, const std::string& outputDir)
{
    fs::path fp(inputFile);
    if (!fs::exists(fp)) {
        return Status::FILE_NOT_EXISTS;
    }
    if (!fs::is_regular_file(fp)) {
        return Status::NOT_FILE;
    }
    fs::path dir(outputDir);
    if (!fs::exists(dir)) {
        bool isOk = fs::create_directory(dir);
        if (!isOk) {
            return Status::DIR_CREATE_ERR;
        }
    } else {
        if (!fs::is_directory(dir)) {
            return Status::NOT_DIR;
        }
    }

    uint64_t fileSize = fs::file_size(fp);
    size_t fileCnt = fileSize / K1_BUFF;
    if (fileSize % K1_BUFF != 0) {
        fileCnt += 1;
    }

    std::ifstream fi(fp, std::ios::in | std::ios::binary);
    if (!fi.is_open()) {
        return Status::FILE_OPEN_ERR;
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
    return Status::OK;
}

Status recoveryFile(const std::string& inputDir, const std::string& outputFile)
{
    fs::path dirPath(inputDir);
    if (!fs::exists(dirPath)) {
        return Status::DIR_NOT_EXISTS;
    }
    if (!fs::is_directory(dirPath)) {
        return Status::NOT_DIR;
    }
    fs::path fp(outputFile);
    if (fs::exists(fp)) {
        return Status::FILE_ALREADY_EXISTS;
    }

    std::ofstream fo(fp, std::ios::out | std::ios::binary | std::ios::app);
    if (!fo.is_open()) {
        return Status::FILE_OPEN_ERR;
    }

    fs::directory_iterator itDir(dirPath);
    for (const fs::directory_entry& p : itDir) {
        std::cout << p.path() << std::endl;
        cv::Mat img = cv::imread(p.path().string());
        if (!(img.rows == K1_HEIGH && img.cols == K1_WIDTH)) {
            continue;
        }
        uint64_t dataSz = 0;
        std::memcpy(&dataSz, img.data, sizeof(dataSz));
        fo.write((char*)&img.data[8], dataSz);
    }
    return Status::OK;
}

}

#endif
