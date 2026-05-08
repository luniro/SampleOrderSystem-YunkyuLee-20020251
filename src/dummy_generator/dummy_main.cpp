#include "dummy_generator/dummy_generator.hpp"
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif

int main() {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
    GeneratorConfig config;
    config.output_dir = ".";
    generate_dummy_data(config);
    std::cout << "더미 데이터 생성 완료.\n";
    return 0;
}
