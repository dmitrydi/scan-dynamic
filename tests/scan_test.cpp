#include "parse.hpp"
#include "scan.hpp"
#include "types.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <type_traits>

TEST(ScanTest, Parse) {
    auto input = std::string_view("I want to sum 42 and 3.14 numbers.");
    auto format = std::string_view("I want to sum {} and {%f} numbers.");
    auto res = stdx::details::parse_sources<>(input, format);
    ASSERT_TRUE(res.has_value());
}

TEST(ScanTest, SimpleTest) {
    auto result = stdx::scan<std::string>("number", "{}");
    ASSERT_TRUE(result);
    ASSERT_EQ(std::get<0>(result->data), std::string("number"));
}

TEST(ScanTest, TestConstInt) {
    auto result = stdx::scan<const int>("4", "{%d}");
    ASSERT_TRUE(result);
    ASSERT_EQ(std::get<0>(result->data), 4);
}

TEST(ScanTest, TestConstString) {
    auto result = stdx::scan<const std::string>("text", "{}");
    ASSERT_TRUE(result);
    ASSERT_TRUE((std::is_same_v<std::remove_reference_t<decltype(result->value<0>())>, const std::string>));
    ASSERT_EQ(std::get<0>(result->data), "text");
}

TEST(ScanTest, TestTwoNumbers) {
    auto result = stdx::scan<int, double>("I want to sum 42 and 3.14 numbers.", "I want to sum {} and {%f} numbers.");
    ASSERT_TRUE(result);
    ASSERT_EQ(std::get<0>(result->data), int(42));
    EXPECT_DOUBLE_EQ(std::get<1>(result->data), double(3.14));
}

TEST(ScanTest, TestTwoNumbersFail) {
    auto result = stdx::scan<int, double>("I want to sum 4s2 and 3.14 numbers.", "I want to sum {} and {%f} numbers.");
    ASSERT_FALSE(result);
}

TEST(ScanTest, TestInvalidType) {
    auto result =
        stdx::scan<int, std::string>("I want to sum 42 and 3.14 numbers.", "I want to sum {} and {%f} numbers.");
    ASSERT_FALSE(result);
}

TEST(ScanTest, TestInvalidIntegerType) {
    auto result = stdx::scan<int, float>("I want to sum 42 and 3.14 numbers.", "I want to sum {%u} and {%f} numbers.");
    ASSERT_FALSE(result);
}

TEST(ScanTest, TestInvalidFloatingType) {
    auto result = stdx::scan<int, float>("I want to sum 42 and 3.14 numbers.", "I want to sum {%d} and {%d} numbers.");
    ASSERT_FALSE(result);
}

TEST(ScanTest, TestSignedOK) {
    auto result = stdx::scan<int>("I want to get -42.", "I want to get {%d}.");
    ASSERT_TRUE(result);
    ASSERT_EQ(result->value<0>(), -42);
}

TEST(ScanTest, TestUnisgned) {
    auto result = stdx::scan<unsigned>("I want to get -42.", "I want to get {%u}.");
    ASSERT_FALSE(result);
}

TEST(ScanTest, TestPointerType) {
    auto result = stdx::scan<int *>("I want to get 1.", "I want to get {}.");
    ASSERT_FALSE(result);
}

enum class Foo {
    one = 1,
    two = 2,
};

TEST(ScanTest, TestUnsupportedType) {
    auto result = stdx::scan<Foo>("I want to get 1.", "I want to get {}.");
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error().message, "type not supported");
}
