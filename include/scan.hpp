#pragma once

#include "parse.hpp"
#include "types.hpp"
#include <expected>
#include <numeric>
#include <string_view>
#include <vector>

namespace stdx {

template <typename... Args, std::size_t... Is>
auto scan_impl(const std::vector<std::string_view> &inputs, const std::vector<std::string_view> &formats,
               std::index_sequence<Is...>) -> std::expected<details::scan_result<Args...>, details::scan_error> {
    auto parse_results = std::make_tuple(details::parse_value_with_format<Args>(inputs[Is], formats[Is])...);

    bool has_error = false;
    details::scan_error error;

    (
        [&]() {
            if (!has_error && !std::get<Is>(parse_results)) {
                has_error = true;
                error = std::get<Is>(parse_results).error();
            }
        }(),
        ...);

    if (has_error) {
        return std::unexpected(error);
    }

    return details::scan_result<Args...>(std::move(*std::get<Is>(parse_results))...);
}

// замените болванку функции scan на рабочую версию
template <typename... Ts>
std::expected<details::scan_result<Ts...>, details::scan_error> scan(std::string_view input, std::string_view format) {
    auto parsed = details::parse_sources(input, format);
    if (!parsed) {
        return std::unexpected(parsed.error());
    }
    if (sizeof...(Ts) != parsed->first.size() || sizeof...(Ts) != parsed->second.size()) {
        return std::unexpected(details::scan_error("wrong pack size"));
    }

    return scan_impl<Ts...>(parsed->second, parsed->first, std::index_sequence_for<Ts...>{});
}

}  // namespace stdx
