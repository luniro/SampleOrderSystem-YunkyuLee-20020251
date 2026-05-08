#pragma once
#include <string>
#include <vector>

class TablePrinter {
public:
    explicit TablePrinter(std::vector<std::string> headers);

    void add_row(std::vector<std::string> row);
    void print() const;           // existing: full output, no paging
    void print_paged() const;     // new: console-height-based paging

private:
    std::vector<std::string>              headers_;
    std::vector<std::vector<std::string>> rows_;
};
