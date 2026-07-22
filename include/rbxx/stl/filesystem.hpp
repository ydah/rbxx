#pragma once

#include <rbxx/stl/detail.hpp>

#include <filesystem>

namespace rbxx {
namespace detail {
template <> struct is_non_bindable_class<std::filesystem::path> : std::true_type {};
} // namespace detail

template <> struct type_caster<std::filesystem::path> {
  static constexpr std::string_view name = "String path";
  static std::filesystem::path load(value input) {
    return std::filesystem::path{from_ruby<std::string>(input)};
  }
  static value dump(const std::filesystem::path& input) { return to_ruby(input.string()); }
  static bool matches(value input) noexcept { return type_caster<std::string>::matches(input); }
};

} // namespace rbxx
