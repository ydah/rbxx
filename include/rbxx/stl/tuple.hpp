#pragma once

#include <rbxx/stl/detail.hpp>

#include <tuple>
#include <utility>

namespace rbxx {
namespace detail {
template <typename First, typename Second>
struct is_non_bindable_class<std::pair<First, Second>> : std::true_type {};
template <typename... Items> struct is_non_bindable_class<std::tuple<Items...>> : std::true_type {};

inline void require_tuple_length(VALUE array, long expected) {
  if (RARRAY_LEN(array) == expected) {
    return;
  }
  std::string message = "rbxx: expected Array tuple of length ";
  message += std::to_string(expected);
  message += ", got ";
  message += std::to_string(RARRAY_LEN(array));
  throw ruby_error(make_exception(rb_eArgError, message.c_str()));
}

template <typename Tuple, std::size_t... Index>
Tuple load_tuple(VALUE array, std::index_sequence<Index...>) {
  return Tuple{from_ruby<std::tuple_element_t<Index, Tuple>>(
      value{RARRAY_AREF(array, static_cast<long>(Index))})...};
}

template <typename Tuple, std::size_t... Index>
value dump_tuple(const Tuple& input, std::index_sequence<Index...>) {
  return value{protect([&input] {
    VALUE result = rb_ary_new_capa(static_cast<long>(sizeof...(Index)));
    (rb_ary_push(result, to_ruby(std::get<Index>(input)).raw()), ...);
    return result;
  })};
}
} // namespace detail

template <typename First, typename Second> struct type_caster<std::pair<First, Second>> {
  using pair_type = std::pair<First, Second>;
  static constexpr std::string_view name = "Array(2)";
  static pair_type load(value input) {
    VALUE array = detail::coerce_array(input);
    detail::require_tuple_length(array, 2);
    return {from_ruby<First>(value{RARRAY_AREF(array, 0)}),
            from_ruby<Second>(value{RARRAY_AREF(array, 1)})};
  }
  static value dump(const pair_type& input) {
    return detail::dump_tuple(input, std::index_sequence<0, 1>{});
  }
  static bool matches(value input) noexcept {
    return input.is_array() && RARRAY_LEN(input.raw()) == 2;
  }
};

template <typename... Items> struct type_caster<std::tuple<Items...>> {
  using tuple_type = std::tuple<Items...>;
  static constexpr std::string_view name = "Array tuple";
  static tuple_type load(value input) {
    VALUE array = detail::coerce_array(input);
    detail::require_tuple_length(array, static_cast<long>(sizeof...(Items)));
    return detail::load_tuple<tuple_type>(array, std::index_sequence_for<Items...>{});
  }
  static value dump(const tuple_type& input) {
    return detail::dump_tuple(input, std::index_sequence_for<Items...>{});
  }
  static bool matches(value input) noexcept {
    return input.is_array() && RARRAY_LEN(input.raw()) == static_cast<long>(sizeof...(Items));
  }
};

} // namespace rbxx
