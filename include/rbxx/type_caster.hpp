#pragma once

#include <rbxx/protect.hpp>

#include <concepts>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace rbxx {

/// @brief Customization point for conversion between Ruby and C++ values.
/// @code template <> struct rbxx::type_caster<MyType> { /* load/dump/matches */ }; @endcode
template <typename T, typename Enable = void> struct type_caster;

/// @brief True when T can be converted to a Ruby value.
template <typename T>
concept to_ruby_convertible = requires(T&& input) {
  {
    type_caster<std::remove_cvref_t<T>>::dump(std::forward<T>(input))
  } -> std::convertible_to<value>;
};

/// @brief True when a Ruby value can be converted to T.
template <typename T>
concept from_ruby_convertible =
    requires(value input) { type_caster<std::remove_cvref_t<T>>::load(input); };

namespace detail {

template <typename T> constexpr std::string_view type_name() noexcept {
#if defined(_MSC_VER)
  constexpr std::string_view signature = __FUNCSIG__;
  constexpr std::string_view prefix = "type_name<";
  const auto start = signature.find(prefix) + prefix.size();
  return signature.substr(start, signature.find(">(void)", start) - start);
#else
  constexpr std::string_view signature = __PRETTY_FUNCTION__;
  constexpr std::string_view prefix = "T = ";
  const auto start = signature.find(prefix) + prefix.size();
  const auto end = signature.find_first_of("];", start);
  return signature.substr(start, end - start);
#endif
}

[[noreturn]] inline void throw_type_error(const char* expected, value actual) {
  const char* actual_name = rb_obj_classname(actual.raw());
  std::string message = "rbxx: expected ";
  message += expected;
  message += ", got ";
  message += actual_name;
  throw ruby_error(make_exception(rb_eTypeError, message.c_str()));
}

template <typename Integer> Integer load_integer(value input) {
  if constexpr (std::is_signed_v<Integer>) {
    const auto converted = protect(rb_num2ll, input.raw());
    constexpr auto minimum = static_cast<LONG_LONG>(std::numeric_limits<Integer>::min());
    constexpr auto maximum = static_cast<LONG_LONG>(std::numeric_limits<Integer>::max());
    if (converted < minimum || converted > maximum) {
      throw std::range_error("rbxx: Integer is outside the target C++ signed integer range");
    }
    return static_cast<Integer>(converted);
  } else {
    const auto converted = protect(rb_num2ull, input.raw());
    constexpr auto maximum = static_cast<unsigned LONG_LONG>(std::numeric_limits<Integer>::max());
    if (converted > maximum) {
      throw std::range_error("rbxx: Integer is outside the target C++ unsigned integer range");
    }
    return static_cast<Integer>(converted);
  }
}

template <typename Integer> value dump_integer(Integer input) {
  if constexpr (std::is_signed_v<Integer>) {
    return value{protect(rb_ll2inum, static_cast<LONG_LONG>(input))};
  } else {
    return value{protect(rb_ull2inum, static_cast<unsigned LONG_LONG>(input))};
  }
}

inline const char* checked_string_pointer(value input) {
  if (!input.is_string()) {
    throw_type_error("String", input);
  }
  VALUE raw = input.raw();
  return protect([raw]() mutable {
    VALUE string = raw;
    return StringValueCStr(string);
  });
}

inline value dump_utf8(const char* bytes, std::size_t size) {
  if (size > static_cast<std::size_t>(std::numeric_limits<long>::max())) {
    throw std::length_error("rbxx: String is too large for CRuby");
  }
  return value{protect(rb_utf8_str_new, bytes, static_cast<long>(size))};
}

} // namespace detail

template <typename T>
struct type_caster<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>> {
  static constexpr std::string_view name = "Integer";

  /// @brief Loads a range-checked C++ integer.
  static T load(value input) { return detail::load_integer<T>(input); }

  /// @brief Dumps a C++ integer as a Ruby Integer.
  static value dump(T input) { return detail::dump_integer(input); }

  /// @brief Returns whether the Ruby value is an Integer.
  static bool matches(value input) noexcept { return input.is_integer(); }
};

template <> struct type_caster<bool> {
  static constexpr std::string_view name = "Boolean";

  static bool load(value input) {
    if (!input.is_bool()) {
      detail::throw_type_error("true or false", input);
    }
    return input.raw() == Qtrue;
  }

  static value dump(bool input) noexcept { return value{input ? Qtrue : Qfalse}; }
  static bool matches(value input) noexcept { return input.is_bool(); }
};

template <> struct type_caster<float> {
  static constexpr std::string_view name = "Float";
  static float load(value input) { return static_cast<float>(protect(rb_num2dbl, input.raw())); }
  static value dump(float input) {
    return value{protect(rb_float_new, static_cast<double>(input))};
  }
  static bool matches(value input) noexcept { return input.is_float() || input.is_integer(); }
};

template <> struct type_caster<double> {
  static constexpr std::string_view name = "Float";
  static double load(value input) { return protect(rb_num2dbl, input.raw()); }
  static value dump(double input) { return value{protect(rb_float_new, input)}; }
  static bool matches(value input) noexcept { return input.is_float() || input.is_integer(); }
};

template <> struct type_caster<const char*> {
  static constexpr std::string_view name = "String";
  static const char* load(value input) { return detail::checked_string_pointer(input); }
  static value dump(const char* input) {
    if (input == nullptr) {
      detail::throw_type_error("non-null C string", value{Qnil});
    }
    return value{protect(rb_utf8_str_new_cstr, input)};
  }
  static bool matches(value input) noexcept { return input.is_string(); }
};

template <> struct type_caster<std::string> {
  static constexpr std::string_view name = "String";
  static std::string load(value input) {
    return std::string(detail::checked_string_pointer(input));
  }
  static value dump(const std::string& input) {
    return detail::dump_utf8(input.data(), input.size());
  }
  static bool matches(value input) noexcept { return input.is_string(); }
};

template <> struct type_caster<std::string_view> {
  static constexpr std::string_view name = "String";
  static std::string_view load(value input) {
    const char* bytes = detail::checked_string_pointer(input);
    return std::string_view(bytes, static_cast<std::size_t>(RSTRING_LEN(input.raw())));
  }
  static value dump(std::string_view input) {
    return detail::dump_utf8(input.data(), input.size());
  }
  static bool matches(value input) noexcept { return input.is_string(); }
};

template <> struct type_caster<value> {
  static constexpr std::string_view name = "Object";
  static value load(value input) noexcept { return input; }
  static value dump(value input) noexcept { return input; }
  static bool matches(value) noexcept { return true; }
};

template <> struct type_caster<object> {
  static constexpr std::string_view name = "Object";
  static object load(value input) { return object{input}; }
  static value dump(const object& input) noexcept { return input.get(); }
  static bool matches(value) noexcept { return true; }
};

/// @brief Converts a C++ value to Ruby with an actionable missing-caster diagnostic.
/// @code rbxx::value result = rbxx::to_ruby(42); @endcode
template <typename T> value to_ruby(T&& input) {
  using converted_type = std::remove_cvref_t<T>;
  static_assert(to_ruby_convertible<converted_type>,
                "rbxx: type has no type_caster; bind it with def_class<T>() or specialize "
                "rbxx::type_caster<T>");
  return type_caster<converted_type>::dump(std::forward<T>(input));
}

/// @brief Converts a Ruby value to C++ with an actionable missing-caster diagnostic.
/// @code int result = rbxx::from_ruby<int>(ruby_value); @endcode
template <typename T> decltype(auto) from_ruby(value input) {
  using converted_type = std::remove_cvref_t<T>;
  static_assert(from_ruby_convertible<converted_type>,
                "rbxx: type has no type_caster; bind it with def_class<T>() or specialize "
                "rbxx::type_caster<T>");
  return type_caster<converted_type>::load(input);
}

} // namespace rbxx
