#include "FileCamouflage.hpp"
#include <iostream>

int main(int argc, char* argv[])
{
    std::cout << "文件伪装" << std::endl;
    // if (argc != 3) {
    //     std::cout << "参数错误" << std::endl;
    //     return 0;
    // }
    zwn::convertFiltToImage("D:/Temp/v2rayN-Core.zip", "D:/Temp/Images");
    return 0;
}
