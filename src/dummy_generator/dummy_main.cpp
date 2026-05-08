#include "dummy_generator/dummy_generator.hpp"
#include <iostream>

int main() {
    GeneratorConfig config;
    config.output_dir = ".";
    generate_dummy_data(config);
    std::cout << "더미 데이터 생성 완료.\n";
    return 0;
}
