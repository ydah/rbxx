#pragma once

#include <rbxx/stl/detail.hpp>

#include <array>
#include <sstream>

namespace rbxx {
namespace detail {
template <typename T, std::size_t Size>
struct is_non_bindable_class<std::array<T, Size>> : std::true_type {};
} // namespace detail

template <typename T, std::size_t Size> struct type_caster<std::array<T, Size>> {
  static constexpr std::string_view name = "Array";

  static std::array<T, Size> load(value input) {
    VALUE array = detail::coerce_array(input);
    if (RARRAY_LEN(array) != static_cast<long>(Size)) {
      std::ostringstream message;
      message << "rbxx: expected Array of length " << Size << ", got " << RARRAY_LEN(array);
      throw ruby_error(detail::make_exception(rb_eArgError, message.str().c_str()));
    }
    return load_elements(array, std::make_index_sequence<Size>{});
  }

  static value dump(const std::array<T, Size>& input) { return detail::dump_array_range(input); }

  static bool matches(value input) noexcept {
    return input.is_array() && RARRAY_LEN(input.raw()) == static_cast<long>(Size);
  }

private:
  template <std::size_t... Index>
  static std::array<T, Size> load_elements(VALUE array, std::index_sequence<Index...>) {
    return {from_ruby<T>(value{RARRAY_AREF(array, static_cast<long>(Index))})...};
  }
};

} // namespace rbxx
