#pragma once

#include <rbxx/stl/detail.hpp>

#include <chrono>

namespace rbxx {
namespace detail {
template <typename Rep, typename Period>
struct is_non_bindable_class<std::chrono::duration<Rep, Period>> : std::true_type {};
} // namespace detail

template <typename Rep, typename Period> struct type_caster<std::chrono::duration<Rep, Period>> {
  using duration_type = std::chrono::duration<Rep, Period>;
  static constexpr std::string_view name = "seconds";
  static duration_type load(value input) {
    const auto seconds = std::chrono::duration<double>{from_ruby<double>(input)};
    return std::chrono::duration_cast<duration_type>(seconds);
  }
  static value dump(const duration_type& input) {
    return to_ruby(std::chrono::duration<double>(input).count());
  }
  static bool matches(value input) noexcept { return type_caster<double>::matches(input); }
};

} // namespace rbxx
