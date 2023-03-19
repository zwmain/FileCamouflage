#ifndef _FILE_CAMOUFLAGE_HPP_
#define _FILE_CAMOUFLAGE_HPP_

#include "ThreadPool.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <regex>
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
    FILE_TYPE_UNKOWN,
    NOT_DIR,
    DIR_NOT_EXISTS,
    DIR_CREATE_ERR,
    DATA_ERR,
};

enum class FileType {
    UNKNOWN = 0,
    PNG,
    MP4
};

struct ImageSize {
    size_t width = 0;
    size_t heigh = 0;
    size_t bytes = 0;
    size_t buffs = 0;
};

using ImageStrategy = std::pair<size_t, ImageSize>;
using PathWithId = std::pair<fs::path, size_t>;

const std::vector<ImageSize> IMG_LIST {
    { 7860, 4320, 101865600, 101865592 }, // 8k
    { 3840, 2160, 24883200, 24883192 }, // 4k
    { 2560, 1440, 11059200, 11059192 }, // 2k
    { 1920, 1080, 6220800, 6220792 } // 1k
};
const std::vector<int> PNG_CFG { cv::IMWRITE_PNG_COMPRESSION, 0, cv::IMWRITE_PNG_STRATEGY, cv::IMWRITE_PNG_STRATEGY_DEFAULT };
// C++对零宽断言支持不完整，会崩溃
// const std::regex REG_PNG_FILE_ID { "(?<=.*_)\\d+(?=\\.([pP][nN][gG])$)" };
// 使用子表达式匹配目标数字
const std::regex REG_PNG_FILE_ID { ".*_(\\d+)\\.[pP][nN][gG]$" };

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

std::pair<FileType, size_t> parseFileId(const std::string& fileName);

std::vector<PathWithId> getInputFileList(const fs::path& inputDir);

Status imagesToFile(std::vector<PathWithId>& pathList, const fs::path& outputFile);

std::pair<cv::Mat, size_t> readImageData(const fs::path& inputFile);

bool cmpPathWithId(const PathWithId& a, const PathWithId& b);

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

    std::vector<PathWithId> pathList = getInputFileList(dirPath);
    if (pathList.empty()) {
        return Status::FILE_TYPE_UNKOWN;
    }

    Status stu = imagesToFile(pathList, fp);

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
    auto thPool = ThreadPool::instance();
    std::vector<std::future<Status>> resArr;
    for (size_t i = 0; i < fileCnt; ++i) {
        uint64_t wrSize = remainSize > imgSz.buffs ? imgSz.buffs : remainSize;
        remainSize -= wrSize;
        auto res = thPool->addTask(readFileToImage, inputFile, outpurDir, imgSz, fileName, wrPos, wrSize, i, numWidth);
        resArr.push_back(std::move(res));
        wrPos += wrSize;
    }
    for (auto& res : resArr) {
        Status stu = res.get();
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

std::pair<FileType, size_t> parseFileId(const std::string& fileName)
{
    std::smatch res;
    bool isMatch = std::regex_match(fileName, res, REG_PNG_FILE_ID);
    if (!isMatch || res.size() != 2) {
        return { FileType::UNKNOWN, 0 };
    }
    std::ssub_match subRes = res[1];
    std::string strId = subRes.str();
    size_t fId = std::stoull(strId);
    return { FileType::PNG, fId };
}

std::vector<PathWithId> getInputFileList(const fs::path& inputDir)
{
    std::vector<PathWithId> pathList;
    bool isSorted = true;
    fs::directory_iterator itDir(inputDir);
    for (const fs::directory_entry& p : itDir) {
        auto& inpPath = p.path();
        std::string inpName = inpPath.filename().string();
        auto [fType, fId] = parseFileId(inpName);
        if (fType != FileType::PNG) {
            pathList.clear();
            return pathList;
        }
        if (!pathList.empty() && isSorted) {
            size_t lastId = pathList.back().second;
            if (!(fId > lastId && fId - lastId == 1)) {
                isSorted = false;
            }
        }
        pathList.push_back({ inpPath, fId });
    }
    if (!isSorted) {
        std::sort(pathList.begin(), pathList.end(), cmpPathWithId);
    }
    return pathList;
}

Status imagesToFile(std::vector<PathWithId>& pathList, const fs::path& outputFile)
{
    std::ofstream fo(outputFile, std::ios::out | std::ios::binary | std::ios::app);
    if (!fo.is_open()) {
        return Status::FILE_OPEN_ERR;
    }
    auto thPool = ThreadPool::instance();
    std::vector<std::future<std::pair<cv::Mat, size_t>>> resArr;
    for (auto& [inpPath, fId] : pathList) {
        auto res = thPool->addTask(readImageData, inpPath);
        resArr.push_back(std::move(res));
    }
    Status stu = Status::OK;
    for (auto& res : resArr) {
        auto [img, dataSz] = res.get();
        if (img.empty()) {
            stu = Status::DATA_ERR;
            break;
        }
        fo.write((char*)&img.data[8], dataSz);
    }
    fo.close();

    if (stu != Status::OK && fs::exists(outputFile)) {
        fs::remove(outputFile);
    }
    return stu;
}

std::pair<cv::Mat, size_t> readImageData(const fs::path& inputFile)
{
    cv::Mat img = cv::imread(inputFile.string());
    size_t imgBytes = img.total() * img.elemSize();
    if (img.empty() || imgBytes < 8) {
        return { cv::Mat(), 0 };
    }
    uint64_t dataSz = 0;
    std::memcpy(&dataSz, img.data, sizeof(dataSz));
    return { img, dataSz };
}

bool cmpPathWithId(const PathWithId& a, const PathWithId& b)
{
    return a.second < b.second;
}

}

#endif
