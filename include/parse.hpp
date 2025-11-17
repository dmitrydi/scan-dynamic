#pragma once

#include "types.hpp"
#include <algorithm>
#include <charconv>
#include <concepts>
#include <expected>
#include <format>
#include <source_location>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace stdx::details {

// здесь ваш код
template <typename T>
std::expected<T, scan_error> do_parse(std::string_view input) {
    return std::unexpected(scan_error("type not supported"));
}

template <typename T>
concept IsValue = !std::is_pointer_v<T> && !std::is_reference_v<T>;

template <typename T>
concept UnsignedFixedIntegral =
    std::is_same_v<std::remove_cv_t<T>, uint8_t> || std::is_same_v<std::remove_cv_t<T>, uint16_t> ||
    std::is_same_v<std::remove_cv_t<T>, uint32_t> || std::is_same_v<std::remove_cv_t<T>, uint64_t>;

template <typename T>
concept SignedFixedIntegral =
    std::is_same_v<std::remove_cv_t<T>, int8_t> || std::is_same_v<std::remove_cv_t<T>, int16_t> ||
    std::is_same_v<std::remove_cv_t<T>, int32_t> || std::is_same_v<std::remove_cv_t<T>, int64_t>;

template <typename T>
concept FixedIntegral = SignedFixedIntegral<T> || UnsignedFixedIntegral<T>;

template <typename T>
concept Number = FixedIntegral<T> || std::floating_point<T>;

template <typename T>
concept RemoveCVString = std::is_same_v<std::remove_cv_t<T>, std::string>;

template <typename T>
concept RemoveCVStringView = std::is_same_v<std::remove_cv_t<T>, std::string_view>;

template <Number T>
std::expected<T, scan_error> do_parse(std::string_view input) {
    std::remove_const_t<T> dummy;
    if (auto result = std::from_chars(input.begin(), input.end(), dummy);
        result.ec == std::errc{} && result.ptr == input.data() + input.size()) {
        return T(dummy);
    }
    return std::unexpected(scan_error(std::format("could not convert {}", input)));
}

template <RemoveCVStringView T>
inline std::expected<T, scan_error> do_parse(std::string_view input) {
    return input;
}

template <RemoveCVString T>
inline std::expected<T, scan_error> do_parse(std::string_view input) {
    return T(input);
}

// static const std::unordered_set<char> kFromats =
static constexpr int kNum = 4;
static constexpr std::array<char, kNum> kFormats = {'d', 's', 'u', 'f'};

// Функция для парсинга значения с учетом спецификатора формата
template <typename T>
std::expected<T, scan_error> parse_value_with_format(std::string_view input, std::string_view fmt) {
    // здесь ваш код
    if (!IsValue<T>) {
        return std::unexpected(scan_error("pointers and references are forbidden"));
    }
    if (fmt.empty()) {
        return do_parse<T>(input);
    }
    if (fmt.size() != 2) {
        return std::unexpected(scan_error("invalid format"));
    }
    auto f = fmt[1];
    if (auto it = std::find(kFormats.begin(), kFormats.end(), f); it == kFormats.end()) {
        return std::unexpected(scan_error(std::format("unknown format specifier", fmt)));
    }
    switch (f) {
    case 'd':
        if (!FixedIntegral<T>) {
            return std::unexpected(scan_error(std::format("%d format specified but template type is not integral")));
        }
        break;
    case 's':
        if (!std::is_same_v<T, std::string> && !std::is_same_v<T, std::string_view>) {
            return std::unexpected(scan_error(std::format("%s format specified but template type is not std::string")));
        }
        break;
    case 'u':
        if (!UnsignedFixedIntegral<T>) {
            return std::unexpected(
                scan_error(std::format("%u format specified but template type is not unsigned integral")));
        }
        break;
    case 'f':
        if (!std::floating_point<T>) {
            return std::unexpected(scan_error(std::format("%f format specified but template type is not float")));
        }
        break;
    default:
        return std::unexpected(scan_error(std::format("unknown format specifier", fmt)));
    }
    return do_parse<T>(input);
}

// Функция для проверки корректности входных данных и выделения из обеих строк интересующих данных для парсинга
template <typename... Ts>
std::expected<std::pair<std::vector<std::string_view>, std::vector<std::string_view>>, scan_error>
parse_sources(std::string_view input, std::string_view format) {
    std::vector<std::string_view> format_parts;  // Части формата между {}
    std::vector<std::string_view> input_parts;
    size_t start = 0;
    while (true) {
        size_t open = format.find('{', start);
        if (open == std::string_view::npos) {
            break;
        }
        size_t close = format.find('}', open);
        if (close == std::string_view::npos) {
            break;
        }

        // Если между предыдущей } и текущей { есть текст,
        // проверяем его наличие во входной строке
        if (open > start) {
            std::string_view between = format.substr(start, open - start);
            auto pos = input.find(between);
            if (input.size() < between.size() || pos == std::string_view::npos) {
                return std::unexpected(scan_error{"Unformatted text in input and format string are different"});
            }
            if (start != 0) {
                input_parts.emplace_back(input.substr(0, pos));
            }

            input = input.substr(pos + between.size());
        }

        // Сохраняем спецификатор формата (то, что между {})
        format_parts.push_back(format.substr(open + 1, close - open - 1));
        start = close + 1;
    }

    // Проверяем оставшийся текст после последней }
    if (start < format.size()) {
        std::string_view remaining_format = format.substr(start);
        auto pos = input.find(remaining_format);
        if (input.size() < remaining_format.size() || pos == std::string_view::npos) {
            return std::unexpected(scan_error{"Unformatted text in input and format string are different"});
        }
        input_parts.emplace_back(input.substr(0, pos));
        input = input.substr(pos + remaining_format.size());
    } else {
        input_parts.emplace_back(input);
    }
    return std::pair{format_parts, input_parts};
}

}  // namespace stdx::details