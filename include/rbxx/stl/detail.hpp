#pragma once

#include <rbxx/data_object.hpp>

#include <limits>
#include <string>
#include <type_traits>

namespace rbxx::detail {

inline VALUE coerce_array(value input) {
  VALUE converted = protect(rb_check_array_type, input.raw());
  if (NIL_P(converted)) {
    throw_type_error("Array or object responding to #to_ary", input);
  }
  return converted;
}

inline VALUE coerce_hash(value input) {
  VALUE converted = protect(rb_check_hash_type, input.raw());
  if (NIL_P(converted)) {
    throw_type_error("Hash or object responding to #to_hash", input);
  }
  return converted;
}

template <typename Range> value dump_array_range(const Range& input) {
  const auto size = input.size();
  if (size > static_cast<std::size_t>(std::numeric_limits<long>::max())) {
    throw std::length_error("rbxx: container is too large for a Ruby Array");
  }
  return value{protect([&input, size] {
    VALUE result = rb_ary_new_capa(static_cast<long>(size));
    for (const auto& element : input) {
      using element_type = std::remove_cvref_t<decltype(element)>;
      if constexpr (std::is_floating_point_v<element_type>) {
        rb_ary_push(result, rb_float_new(static_cast<double>(element)));
      } else {
        rb_ary_push(result, to_ruby(element).raw());
      }
    }
    return result;
  })};
}

template <typename Map> value dump_hash_range(const Map& input) {
  return value{protect([&input] {
    VALUE result = rb_hash_new();
    for (const auto& [key, mapped] : input) {
      rb_hash_aset(result, to_ruby(key).raw(), to_ruby(mapped).raw());
    }
    return result;
  })};
}

inline VALUE hash_pairs(VALUE hash) {
  return protect([hash] { return rb_funcall(hash, rb_intern("to_a"), 0); });
}

} // namespace rbxx::detail
