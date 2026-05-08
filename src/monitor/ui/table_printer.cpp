#include "ui/table_printer.hpp"
#include <algorithm>
#include <iostream>

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
