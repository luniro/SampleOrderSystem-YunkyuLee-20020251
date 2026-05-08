#include "mvc/input_util.hpp"

#include <iostream>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace InputUtil {

int read_int(const std::string& prompt,
             int min_val,
             int max_val,
             const std::string& error_msg) {
    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) {
            // EOF
            return min_val;
        }
        try {
            std::size_t pos = 0;
            int val = std::stoi(line, &pos);
            // Ensure entire string was consumed (no trailing garbage)
            // Skip any trailing whitespace
            while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) {
                ++pos;
            }
            if (pos != line.size()) {
                throw std::invalid_argument("trailing chars");
            }
            if (val < min_val || val > max_val) {
                std::cout << error_msg << '\n';
                continue;
            }
            return val;
        } catch (const std::invalid_argument&) {
            std::cout << error_msg << '\n';
        } catch (const std::out_of_range&) {
            std::cout << error_msg << '\n';
        }
    }
}

std::string read_nonempty(const std::string& prompt,
                          const std::string& error_msg) {
    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) {
            // EOF
            return "";
        }
        // Empty or whitespace-only → cancel signal (FR-U-04)
        bool all_space = std::all_of(line.begin(), line.end(),
                                     [](unsigned char c) { return std::isspace(c); });
        if (all_space) {
            return "";
        }
        return line;
    }
}

} // namespace InputUtil
