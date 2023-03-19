#include "FileCamouflage.hpp"
#include <chrono>
#include <iostream>

int main(int argc, char* argv[])
{
    zwn::ThreadPool::instance()->start();
    std::cout << "文件伪装" << std::endl;
    auto curTime = std::chrono::system_clock::now();
    zwn::disguiseFile("D:/ProgramFiles/v2rayN.7z", "D:/Temp/Images");
    auto midTime = std::chrono::system_clock::now();
    auto durTime = midTime - curTime;
    size_t durVal = std::chrono::duration_cast<std::chrono::milliseconds>(durTime).count();
    std::cout << "disguiseFile: " << durVal << std::endl;
    zwn::recoveryFile("D:/Temp/Images", "D:/Temp/v2rayN.7z");
    auto endTime = std::chrono::system_clock::now();
    durTime = endTime - midTime;
    durVal = std::chrono::duration_cast<std::chrono::milliseconds>(durTime).count();
    std::cout << "recoveryFile: " << durVal << std::endl;
    return 0;
}
