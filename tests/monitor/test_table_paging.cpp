#include <gtest/gtest.h>
#include "ui/table_printer.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Helper to redirect cin/cout
struct StreamRedirect {
    std::streambuf* old_cin;
    std::streambuf* old_cout;
    std::istringstream in_buf;
    std::ostringstream out_buf;

    explicit StreamRedirect(const std::string& input)
        : in_buf(input) {
        old_cin  = std::cin.rdbuf(in_buf.rdbuf());
        old_cout = std::cout.rdbuf(out_buf.rdbuf());
    }

    ~StreamRedirect() {
        std::cin.rdbuf(old_cin);
        std::cout.rdbuf(old_cout);
    }

    std::string output() const { return out_buf.str(); }
};

// Build a TablePrinter with N data rows (1 col, content "row_i")
static TablePrinter make_table(int num_rows) {
    TablePrinter tp({"Col"});
    for (int i = 0; i < num_rows; ++i) {
        tp.add_row({"row_" + std::to_string(i)});
    }
    return tp;
}

// ============================================================
// TG-TP: TablePrinter paging
// ============================================================

// TC-TP-01: print_paged() with 0 rows — prints header only, returns
TEST(TG_TP, PrintPaged_Empty) {
    TablePrinter tp({"Col"});
    StreamRedirect r("0\n");
    tp.print_paged();
    // Should have printed the header "Col"
    EXPECT_NE(r.output().find("Col"), std::string::npos);
    // No paging prompt (only 0 rows <= page_rows)
    EXPECT_EQ(r.output().find("[n]"), std::string::npos);
}

// TC-TP-02: print_paged() total rows <= page_rows — full output, no prompt
// Console::get_height() returns 24 (fallback) so page_rows = 20
TEST(TG_TP, PrintPaged_FitsOnePage) {
    // 5 rows, page_rows=20 -> fits
    TablePrinter tp = make_table(5);
    StreamRedirect r("");
    tp.print_paged();
    std::string out = r.output();
    EXPECT_NE(out.find("Col"), std::string::npos);
    for (int i = 0; i < 5; ++i) {
        EXPECT_NE(out.find("row_" + std::to_string(i)), std::string::npos);
    }
    // No paging prompt
    EXPECT_EQ(out.find("[n]"), std::string::npos);
}

// TC-TP-03: print_paged() — "0" exits immediately on first page prompt
TEST(TG_TP, PrintPaged_ExitWithZero) {
    // 25 rows > page_rows(20) on a 24-line console
    TablePrinter tp = make_table(25);
    StreamRedirect r("0\n");
    tp.print_paged();
    std::string out = r.output();
    // Prompt shown
    EXPECT_NE(out.find("[n]다음"), std::string::npos);
    // Page indicator (1/2)
    EXPECT_NE(out.find("(1/2)"), std::string::npos);
    // Only first page rows visible (row_0 .. row_19)
    EXPECT_NE(out.find("row_0"), std::string::npos);
    EXPECT_NE(out.find("row_19"), std::string::npos);
    // row_20 should NOT appear (on second page)
    EXPECT_EQ(out.find("row_20"), std::string::npos);
}

// TC-TP-04: print_paged() — navigate to next page with "n"
TEST(TG_TP, PrintPaged_NextPage) {
    TablePrinter tp = make_table(25);
    StreamRedirect r("n\n0\n");
    tp.print_paged();
    std::string out = r.output();
    // Second page contains row_20..row_24
    EXPECT_NE(out.find("row_20"), std::string::npos);
    EXPECT_NE(out.find("row_24"), std::string::npos);
    // Page indicator (2/2)
    EXPECT_NE(out.find("(2/2)"), std::string::npos);
}

// TC-TP-05: print_paged() — navigate with empty input (same as "n")
TEST(TG_TP, PrintPaged_EmptyInputIsNext) {
    TablePrinter tp = make_table(25);
    StreamRedirect r("\n0\n");
    tp.print_paged();
    std::string out = r.output();
    EXPECT_NE(out.find("row_20"), std::string::npos);
}

// TC-TP-06: print_paged() — "p" on first page is ignored (stay on page 1)
TEST(TG_TP, PrintPaged_PreviousOnFirstPageIgnored) {
    TablePrinter tp = make_table(25);
    StreamRedirect r("p\n0\n");
    tp.print_paged();
    std::string out = r.output();
    // Both prompts should still show (1/2) since we never moved
    size_t first_prompt = out.find("(1/2)");
    EXPECT_NE(first_prompt, std::string::npos);
    size_t second_prompt = out.find("(1/2)", first_prompt + 1);
    EXPECT_NE(second_prompt, std::string::npos);
}

// TC-TP-07: print_paged() — "n" on last page is ignored
TEST(TG_TP, PrintPaged_NextOnLastPageIgnored) {
    TablePrinter tp = make_table(25);
    StreamRedirect r("n\nn\n0\n");
    tp.print_paged();
    std::string out = r.output();
    // After "n" from page 1 -> page 2; second "n" on last page ignored -> stays page 2
    size_t first_p2 = out.find("(2/2)");
    EXPECT_NE(first_p2, std::string::npos);
    size_t second_p2 = out.find("(2/2)", first_p2 + 1);
    EXPECT_NE(second_p2, std::string::npos);
}

// TC-TP-08: print_paged() — navigate next then previous
TEST(TG_TP, PrintPaged_NextThenPrevious) {
    TablePrinter tp = make_table(25);
    StreamRedirect r("n\np\n0\n");
    tp.print_paged();
    std::string out = r.output();
    // Third prompt should show page 1 again
    size_t p1_first  = out.find("(1/2)");
    size_t p2        = out.find("(2/2)");
    size_t p1_second = out.find("(1/2)", p2);
    EXPECT_NE(p1_first,  std::string::npos);
    EXPECT_NE(p2,        std::string::npos);
    EXPECT_NE(p1_second, std::string::npos);
}

// TC-TP-09: print_paged() — EOF exits the loop
TEST(TG_TP, PrintPaged_EOFExits) {
    TablePrinter tp = make_table(25);
    // No input at all -> EOF on first prompt
    StreamRedirect r("");
    tp.print_paged();  // must not hang or crash
    std::string out = r.output();
    EXPECT_NE(out.find("[n]"), std::string::npos);
}

// TC-TP-10: print_paged() — unrecognized input re-shows same page
TEST(TG_TP, PrintPaged_UnrecognizedInputIgnored) {
    TablePrinter tp = make_table(25);
    StreamRedirect r("x\n0\n");
    tp.print_paged();
    std::string out = r.output();
    // Should see (1/2) twice
    size_t first  = out.find("(1/2)");
    EXPECT_NE(first, std::string::npos);
    size_t second = out.find("(1/2)", first + 1);
    EXPECT_NE(second, std::string::npos);
}
