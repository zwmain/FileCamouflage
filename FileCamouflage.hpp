#ifndef _FILE_CAMOUFLAGE_HPP_
#define _FILE_CAMOUFLAGE_HPP_

#include <cstring>
#include <filesystem>
#include <fmt/format.h>
#include <cstring>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <string>
#include <utility>
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
    DIR_CREATE_ERR,
    DATA_ERR
};

struct ImageSize {
    size_t width = 0;
    size_t heigh = 0;
    size_t bytes = 0;
    size_t buffs = 0;
};

using ImageStrategy = std::pair<size_t, ImageSize>;

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

Status fileToImages(const fs::path& inputFile, const fs::path& outpurDir, const ImageStrategy& imgSty);

Status readFileToImage(
    const fs::path& inputFile,
    const fs::path& outpurDir,
    const ImageSize& imgSz,
    const std::string& fileName,
    uint64_t rdPos,
    uint64_t rdSize,
    size_t imgId,
    size_t idWidth);

ImageStrategy makeImageStrategy(uint64_t fileSize);

size_t getNumWidth(size_t num);

// ----------------------------------------------------------------------------

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
    ImageStrategy imgSty = makeImageStrategy(fileSize);
    Status stu = fileToImages(fp, dir, imgSty);
    return stu;
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

    Status stu = Status::OK;
    fs::directory_iterator itDir(dirPath);
    for (const fs::directory_entry& p : itDir) {
        std::cout << p.path() << std::endl;
        cv::Mat img = cv::imread(p.path().string());
        size_t imgBytes = img.total() * img.elemSize();
        if (img.empty() || imgBytes < 8) {
            stu = Status::DATA_ERR;
            break;
        }
        uint64_t dataSz = 0;
        std::memcpy(&dataSz, img.data, sizeof(dataSz));
        fo.write((char*)&img.data[8], dataSz);
    }
    fo.close();

    if (stu != Status::OK && fs::exists(fp)) {
        fs::remove(fp);
    }

    return stu;
}

Status fileToImages(const fs::path& inputFile, const fs::path& outpurDir, const ImageStrategy& imgSty)
{
    uint64_t fileSize = fs::file_size(inputFile);
    auto [fileCnt, imgSz] = imgSty;
    const size_t numWidth = getNumWidth(fileCnt);

    std::string fileName = inputFile.filename().string();
    uint64_t remainSize = fileSize;
    uint64_t wrPos = 0;
    for (size_t i = 0; i < fileCnt; ++i) {
        uint64_t wrSize = remainSize > imgSz.buffs ? imgSz.buffs : remainSize;
        remainSize -= wrSize;
        Status stu = readFileToImage(inputFile, outpurDir, imgSz, fileName, wrPos, wrSize, i, numWidth);
        wrPos += wrSize;
        if (stu != Status::OK) {
            return stu;
        }
    }
    return Status::OK;
}

Status readFileToImage(
    const fs::path& inputFile,
    const fs::path& outpurDir,
    const ImageSize& imgSz,
    const std::string& fileName,
    uint64_t rdPos,
    uint64_t rdSize,
    size_t imgId,
    size_t idWidth)
{
    std::ifstream fi(inputFile, std::ios::in | std::ios::binary);
    if (!fi.is_open()) {
        return Status::FILE_OPEN_ERR;
    }
    cv::Mat img = cv::Mat::zeros(imgSz.heigh, imgSz.width, CV_8UC3);
    uchar* data = img.data;
    std::memcpy(data, &rdSize, sizeof(rdSize));
    fi.seekg(rdPos);
    fi.read((char*)&data[8], rdSize);
    std::string outName = fmt::format("{}_{:>0{}}.png", fileName, imgId, idWidth);
    fs::path outPath = outpurDir / outName;
    cv::imwrite(outPath.string(), img, PNG_CFG);
    return Status::OK;
}

ImageStrategy makeImageStrategy(uint64_t fileSize)
{
    ImageSize sz;
    for (auto& imgSz : IMG_LIST) {
        if (fileSize >= imgSz.buffs) {
            sz = imgSz;
            break;
        }
    }
    if (sz.buffs == 0) {
        sz = IMG_LIST.back();
    }
    size_t fileCnt = fileSize / sz.buffs;
    if (fileSize % sz.buffs != 0) {
        fileCnt += 1;
    }
    return { fileCnt, sz };
}

size_t getNumWidth(size_t num)
{
    size_t numW = 1;
    while (num / 10) {
        num /= 10;
        ++numW;
    }
    return numW;
}


}

#endif
