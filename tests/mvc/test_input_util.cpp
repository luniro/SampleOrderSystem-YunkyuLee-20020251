#include <gtest/gtest.h>
#include "mvc/input_util.hpp"

#include <iostream>
#include <sstream>
#include <string>

// Helper: redirect cin/cout for test isolation
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

// ============================================================
// TG-IU: InputUtil
// ============================================================

// TC-IU-01: read_int() — valid input in range
TEST(TG_IU, ReadInt_Valid) {
    StreamRedirect r("3\n");
    int val = InputUtil::read_int("Enter: ", 1, 5);
    EXPECT_EQ(val, 3);
}

// TC-IU-02: read_int() — boundary min
TEST(TG_IU, ReadInt_BoundaryMin) {
    StreamRedirect r("1\n");
    int val = InputUtil::read_int("Enter: ", 1, 5);
    EXPECT_EQ(val, 1);
}

// TC-IU-03: read_int() — boundary max
TEST(TG_IU, ReadInt_BoundaryMax) {
    StreamRedirect r("5\n");
    int val = InputUtil::read_int("Enter: ", 1, 5);
    EXPECT_EQ(val, 5);
}

// TC-IU-04: read_int() — out-of-range then valid
TEST(TG_IU, ReadInt_OutOfRangeThenValid) {
    StreamRedirect r("0\n6\n3\n");
    int val = InputUtil::read_int("Enter: ", 1, 5);
    EXPECT_EQ(val, 3);
    // Should contain error message twice
    EXPECT_NE(r.output().find("잘못된 입력"), std::string::npos);
}

// TC-IU-05: read_int() — non-integer then valid
TEST(TG_IU, ReadInt_NonIntegerThenValid) {
    StreamRedirect r("abc\n2\n");
    int val = InputUtil::read_int("Enter: ", 1, 5);
    EXPECT_EQ(val, 2);
    EXPECT_NE(r.output().find("잘못된 입력"), std::string::npos);
}

// TC-IU-06: read_int() — empty line then valid
TEST(TG_IU, ReadInt_EmptyThenValid) {
    StreamRedirect r("\n4\n");
    int val = InputUtil::read_int("Enter: ", 1, 5);
    EXPECT_EQ(val, 4);
}

// TC-IU-07: read_int() — EOF returns min_val
TEST(TG_IU, ReadInt_EOF) {
    StreamRedirect r("");
    int val = InputUtil::read_int("Enter: ", 1, 5);
    EXPECT_EQ(val, 1);
}

// TC-IU-08: read_int() — custom error message
TEST(TG_IU, ReadInt_CustomErrorMsg) {
    StreamRedirect r("99\n3\n");
    InputUtil::read_int("Enter: ", 1, 5, "custom error");
    EXPECT_NE(r.output().find("custom error"), std::string::npos);
}

// TC-IU-09: read_int() — float input (trailing chars) treated as invalid
TEST(TG_IU, ReadInt_FloatInput) {
    StreamRedirect r("3.5\n2\n");
    int val = InputUtil::read_int("Enter: ", 1, 5);
    EXPECT_EQ(val, 2);
    EXPECT_NE(r.output().find("잘못된 입력"), std::string::npos);
}

// TC-IU-10: read_nonempty() — valid string
TEST(TG_IU, ReadNonempty_Valid) {
    StreamRedirect r("hello\n");
    std::string val = InputUtil::read_nonempty("Enter: ");
    EXPECT_EQ(val, "hello");
}

// TC-IU-11: read_nonempty() — blank line then valid
TEST(TG_IU, ReadNonempty_BlankThenValid) {
    StreamRedirect r("\nhello\n");
    std::string val = InputUtil::read_nonempty("Enter: ");
    EXPECT_EQ(val, "hello");
    EXPECT_NE(r.output().find("빈 값"), std::string::npos);
}

// TC-IU-12: read_nonempty() — spaces-only then valid
TEST(TG_IU, ReadNonempty_SpacesOnlyThenValid) {
    StreamRedirect r("   \nworld\n");
    std::string val = InputUtil::read_nonempty("Enter: ");
    EXPECT_EQ(val, "world");
    EXPECT_NE(r.output().find("빈 값"), std::string::npos);
}

// TC-IU-13: read_nonempty() — EOF returns empty string
TEST(TG_IU, ReadNonempty_EOF) {
    StreamRedirect r("");
    std::string val = InputUtil::read_nonempty("Enter: ");
    EXPECT_EQ(val, "");
}

// TC-IU-14: read_nonempty() — no trimming (leading space preserved)
TEST(TG_IU, ReadNonempty_NoTrimming) {
    StreamRedirect r("  hello  \n");
    std::string val = InputUtil::read_nonempty("Enter: ");
    EXPECT_EQ(val, "  hello  ");
}

// TC-IU-15: read_nonempty() — custom error message
TEST(TG_IU, ReadNonempty_CustomErrorMsg) {
    StreamRedirect r("\nvalue\n");
    InputUtil::read_nonempty("Enter: ", "custom empty error");
    EXPECT_NE(r.output().find("custom empty error"), std::string::npos);
}
