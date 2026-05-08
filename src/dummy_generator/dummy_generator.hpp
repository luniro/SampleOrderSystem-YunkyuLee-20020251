#pragma once
#include <string>

struct GeneratorConfig {
    int         sample_count     = 5;
    int         order_count      = 10;
    int         production_count = 8;
    std::string output_dir       = ".";
};

void generate_dummy_data(const GeneratorConfig& config);
