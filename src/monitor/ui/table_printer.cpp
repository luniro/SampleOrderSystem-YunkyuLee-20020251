#include "ui/table_printer.hpp"
#include "ui/console.hpp"
#include <algorithm>
#include <iostream>
#include <string>

// Returns the terminal display width of a UTF-8 string.
// ASCII = 1 column; Korean/CJK wide characters = 2 columns.
static size_t display_width(const std::string& s) {
    size_t width = 0;
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        uint32_t cp = 0;
        int bytes = 0;
        if (c < 0x80)                        { cp = c;         bytes = 1; }
        else if ((c & 0xE0) == 0xC0)         { cp = c & 0x1F;  bytes = 2; }
        else if ((c & 0xF0) == 0xE0)         { cp = c & 0x0F;  bytes = 3; }
        else if ((c & 0xF8) == 0xF0)         { cp = c & 0x07;  bytes = 4; }
        else                                 { ++i; continue; }
        for (int j = 1; j < bytes && i + static_cast<size_t>(j) < s.size(); ++j)
            cp = (cp << 6) | (static_cast<unsigned char>(s[i + j]) & 0x3F);
        i += static_cast<size_t>(bytes);

        // East Asian Wide / Fullwidth ranges
        bool wide =
            (cp >= 0x1100  && cp <= 0x115F)  ||   // Hangul Jamo
            (cp >= 0x2E80  && cp <= 0x303E)  ||   // CJK Radicals / Kangxi
            (cp >= 0x3040  && cp <= 0x33FF)  ||   // Kana, Bopomofo, Hangul Compat
            (cp >= 0x3400  && cp <= 0x4DBF)  ||   // CJK Extension A
            (cp >= 0x4E00  && cp <= 0x9FFF)  ||   // CJK Unified Ideographs
            (cp >= 0xA000  && cp <= 0xA4CF)  ||   // Yi
            (cp >= 0xAC00  && cp <= 0xD7AF)  ||   // Hangul Syllables
            (cp >= 0xF900  && cp <= 0xFAFF)  ||   // CJK Compatibility Ideographs
            (cp >= 0xFE10  && cp <= 0xFE1F)  ||   // Vertical forms
            (cp >= 0xFE30  && cp <= 0xFE4F)  ||   // CJK Compatibility Forms
            (cp >= 0xFF00  && cp <= 0xFF60)  ||   // Fullwidth / Halfwidth
            (cp >= 0xFFE0  && cp <= 0xFFE6)  ||
            (cp >= 0x20000 && cp <= 0x2FFFD) ||   // CJK Extension B+
            (cp >= 0x30000 && cp <= 0x3FFFD);
        width += wide ? 2u : 1u;
    }
    return width;
}

TablePrinter::TablePrinter(std::vector<std::string> headers)
    : headers_(std::move(headers)) {}

void TablePrinter::add_row(std::vector<std::string> row) {
    rows_.push_back(std::move(row));
}

void TablePrinter::print() const {
    if (headers_.empty()) return;

    size_t cols = headers_.size();
    std::vector<size_t> widths(cols, 0);

    for (size_t c = 0; c < cols; ++c)
        widths[c] = display_width(headers_[c]);
    for (const auto& row : rows_)
        for (size_t c = 0; c < cols && c < row.size(); ++c)
            widths[c] = std::max(widths[c], display_width(row[c]));

    auto print_row = [&](const std::vector<std::string>& cells) {
        for (size_t c = 0; c < cols; ++c) {
            const std::string& val = (c < cells.size()) ? cells[c] : "";
            std::cout << "  " << val;
            if (c + 1 < cols)
                std::cout << std::string(widths[c] - display_width(val), ' ');
        }
        std::cout << '\n';
    };

    print_row(headers_);

    std::cout << "  ";
    for (size_t c = 0; c < cols; ++c) {
        std::cout << std::string(widths[c], '-');
        if (c + 1 < cols) std::cout << "  ";
    }
    std::cout << '\n';

    for (const auto& row : rows_)
        print_row(row);
}

void TablePrinter::print_paged() const {
    if (headers_.empty()) return;

    // Compute column widths using terminal display width
    size_t cols = headers_.size();
    std::vector<size_t> widths(cols, 0);
    for (size_t c = 0; c < cols; ++c)
        widths[c] = display_width(headers_[c]);
    for (const auto& row : rows_)
        for (size_t c = 0; c < cols && c < row.size(); ++c)
            widths[c] = std::max(widths[c], display_width(row[c]));

    auto print_row = [&](const std::vector<std::string>& cells) {
        for (size_t c = 0; c < cols; ++c) {
            const std::string& val = (c < cells.size()) ? cells[c] : "";
            std::cout << "  " << val;
            if (c + 1 < cols)
                std::cout << std::string(widths[c] - display_width(val), ' ');
        }
        std::cout << '\n';
    };

    auto print_separator = [&]() {
        std::cout << "  ";
        for (size_t c = 0; c < cols; ++c) {
            std::cout << std::string(widths[c], '-');
            if (c + 1 < cols) std::cout << "  ";
        }
        std::cout << '\n';
    };

    // Determine page size: H - 4 (header + separator + prompt + margin)
    static constexpr int RESERVED_ROWS = 4;
    int H = Console::get_height();
    int page_rows = H - RESERVED_ROWS;
    if (page_rows < 1) page_rows = 1;

    int total_rows = static_cast<int>(rows_.size());

    // If all rows fit on one page, just print and return
    if (total_rows <= page_rows) {
        print_row(headers_);
        print_separator();
        for (const auto& row : rows_)
            print_row(row);
        return;
    }

    // Multi-page navigation
    int total_pages = (total_rows + page_rows - 1) / page_rows;
    int current_page = 0; // 0-based

    while (true) {
        // Print header + separator
        print_row(headers_);
        print_separator();

        // Print rows for current page
        int start = current_page * page_rows;
        int end   = start + page_rows;
        if (end > total_rows) end = total_rows;
        for (int i = start; i < end; ++i)
            print_row(rows_[static_cast<size_t>(i)]);

        // Prompt
        std::cout << "[n]다음  [p]이전  [0]나가기  ("
                  << (current_page + 1) << "/" << total_pages << "): ";
        std::cout.flush();

        std::string input;
        if (!std::getline(std::cin, input)) {
            break; // EOF
        }

        if (input == "0") {
            break;
        } else if (input == "p") {
            if (current_page > 0) --current_page;
        } else if (input == "n" || input.empty()) {
            if (current_page < total_pages - 1) ++current_page;
        }
        // Other input: ignore (loop re-prints same page)
    }
}
