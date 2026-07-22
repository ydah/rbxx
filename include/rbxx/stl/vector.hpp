#pragma once

#include <rbxx/stl/detail.hpp>

#include <vector>

namespace rbxx {
namespace detail {
template <typename T, typename Allocator>
struct is_non_bindable_class<std::vector<T, Allocator>> : std::true_type {};
} // namespace detail

template <typename T, typename Allocator> struct type_caster<std::vector<T, Allocator>> {
  static constexpr std::string_view name = "Array";

  static std::vector<T, Allocator> load(value input) {
    VALUE array = detail::coerce_array(input);
    const auto length = RARRAY_LEN(array);
    std::vector<T, Allocator> result;
    result.reserve(static_cast<std::size_t>(length));
    for (long index = 0; index < length; ++index) {
      result.push_back(from_ruby<T>(value{RARRAY_AREF(array, index)}));
    }
    return result;
  }

  static value dump(const std::vector<T, Allocator>& input) {
    return detail::dump_array_range(input);
  }

  static bool matches(value input) noexcept { return input.is_array(); }
};

} // namespace rbxx
