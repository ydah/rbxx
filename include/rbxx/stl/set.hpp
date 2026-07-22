#pragma once

#include <rbxx/stl/detail.hpp>

#include <set>

namespace rbxx {
namespace detail {
template <typename T, typename Compare, typename Allocator>
struct is_non_bindable_class<std::set<T, Compare, Allocator>> : std::true_type {};
} // namespace detail

template <typename T, typename Compare, typename Allocator>
struct type_caster<std::set<T, Compare, Allocator>> {
  using set_type = std::set<T, Compare, Allocator>;
  static constexpr std::string_view name = "Array";
  static set_type load(value input) {
    VALUE array = detail::coerce_array(input);
    set_type result;
    const long length = RARRAY_LEN(array);
    for (long index = 0; index < length; ++index) {
      result.insert(from_ruby<T>(value{RARRAY_AREF(array, index)}));
    }
    return result;
  }
  static value dump(const set_type& input) { return detail::dump_array_range(input); }
  static bool matches(value input) noexcept { return input.is_array(); }
};

} // namespace rbxx
