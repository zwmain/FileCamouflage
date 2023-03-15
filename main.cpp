#include "FileCamouflage.hpp"
#include <iostream>

int main(int argc, char* argv[])
{
    std::cout << "文件伪装" << std::endl;
    // if (argc != 3) {
    //     std::cout << "参数错误" << std::endl;
    //     return 0;
    // }
    std::string inp;
    std::cin >> inp;
    std::string out;
    std::cin >> out;
    zwn::convertFiltToImage(inp, out);
    return 0;
}
