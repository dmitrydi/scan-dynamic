#pragma once

#include <string>
#include <tuple>

namespace stdx::details {

// Класс для хранения ошибки неуспешного сканирования

struct scan_error {
    std::string message;
};

// Шаблонный класс для хранения результатов успешного сканирования

template <typename... Ts>
struct scan_result {
    scan_result() = default;
    scan_result(Ts... t) : data(std::make_tuple(std::forward<Ts>(t)...)) {}
    // здесь ваш код
    std::tuple<Ts...> data;
    template <size_t idx>
    decltype(auto) value() {
        return std::get<idx>(data);
    }
};

}  // namespace stdx::details
