#pragma once
#include <string>

namespace InputUtil {
    // Print prompt then read an integer in [min_val, max_val].
    // On invalid input or out-of-range, print error_msg and retry.
    // On EOF, return min_val immediately.
    int read_int(const std::string& prompt,
                 int min_val,
                 int max_val,
                 const std::string& error_msg = "잘못된 입력입니다. 다시 입력해 주세요.");

    // Print prompt then read a non-empty string.
    // Empty or whitespace-only input (including blank Enter) returns "" immediately — cancel signal (FR-U-04).
    // On EOF, also return "".
    // The returned string is the raw input (no trimming).
    std::string read_nonempty(const std::string& prompt,
                              const std::string& error_msg = "빈 값은 입력할 수 없습니다. 다시 입력해 주세요.");
}
