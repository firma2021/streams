#include <array>
#include <charconv>
#include <iomanip>
#include <iostream>
#include <string>

// 辅助函数：打印缓冲区内容
void print_result(const std::to_chars_result& result, char* first, char* last)
{
    if (result.ec == std::errc())
    {
        std::string_view sv(first, result.ptr - first);
        std::cout << "Success: \"" << sv << "\"\n";
    }
    else { std::cout << "Error: " << (result.ec == std::errc::value_too_large ? "value too large" : "invalid argument") << "\n"; }
}

// 测试不同格式的浮点数转换
template <typename T>
void test_to_chars(T value, const std::string& type_name)
{
    std::cout << "\nTesting " << type_name << " value: " << value << "\n";
    std::array<char, 100> buffer;

    // 测试 fixed 格式 (对应 printf 的 %f)
    {
        auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::fixed);
        std::cout << "Fixed format: ";
        print_result(result, buffer.data(), buffer.data() + buffer.size());
    }

    // 测试 scientific 格式 (对应 printf 的 %e)
    {
        auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::scientific);
        std::cout << "Scientific format: ";
        print_result(result, buffer.data(), buffer.data() + buffer.size());
    }

    // 测试 hex 格式 (对应 printf 的 %a，但无前导"0x")
    {
        auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::hex);
        std::cout << "Hex format: ";
        print_result(result, buffer.data(), buffer.data() + buffer.size());
    }

    // 测试 general 格式 (对应 printf 的 %g)
    {
        auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::general);
        std::cout << "General format: ";
        print_result(result, buffer.data(), buffer.data() + buffer.size());
    }
}

int main()
{
    // 测试不同的浮点类型和值

    // 测试 float
    test_to_chars<float>(123.456f, "float");
    test_to_chars<float>(1.23456e-5f, "float");
    test_to_chars<float>(1.23456e5f, "float");

    // 测试 double
    test_to_chars<double>(123.456789, "double");
    test_to_chars<double>(1.23456789e-10, "double");
    test_to_chars<double>(1.23456789e10, "double");

    // 测试 long double
    test_to_chars<long double>(123.456789L, "long double");
    test_to_chars<long double>(1.23456789e-15L, "long double");
    test_to_chars<long double>(1.23456789e15L, "long double");

    // 测试特殊值
    test_to_chars<double>(std::numeric_limits<double>::infinity(), "double infinity");
    test_to_chars<double>(-std::numeric_limits<double>::infinity(), "double -infinity");
    test_to_chars<double>(std::numeric_limits<double>::quiet_NaN(), "double NaN");

    return 0;
}
