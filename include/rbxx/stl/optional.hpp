#pragma once

#include <rbxx/stl/detail.hpp>

#include <optional>

namespace rbxx {
namespace detail {
template <typename T> struct is_non_bindable_class<std::optional<T>> : std::true_type {};
} // namespace detail

template <typename T> struct type_caster<std::optional<T>> {
  static constexpr std::string_view name = "Object or nil";
  static std::optional<T> load(value input) {
    return input.is_nil() ? std::nullopt : std::optional<T>{from_ruby<T>(input)};
  }
  static value dump(const std::optional<T>& input) { return input ? to_ruby(*input) : value{Qnil}; }
  static bool matches(value input) noexcept {
    return input.is_nil() || type_caster<T>::matches(input);
  }
};

} // namespace rbxx
