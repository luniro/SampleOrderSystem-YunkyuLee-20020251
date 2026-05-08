#include "ui/table_printer.hpp"
#include "ui/console.hpp"
#include <algorithm>
#include <iostream>
#include <string>

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
        widths[c] = headers_[c].size();
    for (const auto& row : rows_)
        for (size_t c = 0; c < cols && c < row.size(); ++c)
            widths[c] = std::max(widths[c], row[c].size());

    auto print_row = [&](const std::vector<std::string>& cells) {
        for (size_t c = 0; c < cols; ++c) {
            const std::string& val = (c < cells.size()) ? cells[c] : "";
            std::cout << "  " << val;
            if (c + 1 < cols)
                std::cout << std::string(widths[c] - val.size(), ' ');
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

    // Compute column widths (same logic as print())
    size_t cols = headers_.size();
    std::vector<size_t> widths(cols, 0);
    for (size_t c = 0; c < cols; ++c)
        widths[c] = headers_[c].size();
    for (const auto& row : rows_)
        for (size_t c = 0; c < cols && c < row.size(); ++c)
            widths[c] = std::max(widths[c], row[c].size());

    auto print_row = [&](const std::vector<std::string>& cells) {
        for (size_t c = 0; c < cols; ++c) {
            const std::string& val = (c < cells.size()) ? cells[c] : "";
            std::cout << "  " << val;
            if (c + 1 < cols)
                std::cout << std::string(widths[c] - val.size(), ' ');
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
