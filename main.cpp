#include "FileCamouflage.hpp"
#include <iostream>

int main(int argc, char* argv[])
{
    std::cout << "文件伪装" << std::endl;
    zwn::disguiseFile("D:/ProgramFiles/v2rayN.7z", "D:/Temp/Images");
    zwn::recoveryFile("D:/Temp/Images", "D:/Temp/v2rayN.7z");
    return 0;
}
