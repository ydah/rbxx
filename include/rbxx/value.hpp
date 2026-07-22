#pragma once

#include <rbxx/detail/ruby_include.hpp>

#include <type_traits>

namespace rbxx {

/// @brief A zero-overhead, non-owning view of a Ruby VALUE.
/// @code rbxx::value nil{Qnil}; @endcode
class value {
public:
  /// @brief Constructs a nil value.
  constexpr value() noexcept = default;

  /// @brief Wraps a raw Ruby VALUE without pinning it.
  /// @code auto wrapped = rbxx::value{Qtrue}; @endcode
  explicit constexpr value(VALUE raw) noexcept : raw_(raw) {}

  /// @brief Returns the wrapped Ruby VALUE.
  /// @code VALUE raw = wrapped.raw(); @endcode
  [[nodiscard]] constexpr VALUE raw() const noexcept { return raw_; }

  /// @brief Returns whether this value is nil.
  /// @code if (wrapped.is_nil()) { return; } @endcode
  [[nodiscard]] constexpr bool is_nil() const noexcept { return NIL_P(raw_); }

  /// @brief Returns whether this value is a Ruby boolean.
  [[nodiscard]] constexpr bool is_bool() const noexcept { return raw_ == Qtrue || raw_ == Qfalse; }

  /// @brief Returns whether this value is a Ruby Integer.
  [[nodiscard]] bool is_integer() const noexcept { return RB_INTEGER_TYPE_P(raw_); }

  /// @brief Returns whether this value is a Ruby Float.
  [[nodiscard]] bool is_float() const noexcept { return RB_TYPE_P(raw_, T_FLOAT); }

  /// @brief Returns whether this value is a Ruby String.
  [[nodiscard]] bool is_string() const noexcept { return RB_TYPE_P(raw_, T_STRING); }

  /// @brief Returns whether this value is a Ruby Symbol.
  [[nodiscard]] bool is_symbol() const noexcept { return SYMBOL_P(raw_); }

  /// @brief Returns whether this value is a Ruby Array.
  [[nodiscard]] bool is_array() const noexcept { return RB_TYPE_P(raw_, T_ARRAY); }

  /// @brief Returns whether this value is a Ruby Hash.
  [[nodiscard]] bool is_hash() const noexcept { return RB_TYPE_P(raw_, T_HASH); }

private:
  VALUE raw_ = Qnil;
};

static_assert(std::is_trivially_copyable_v<value>);
static_assert(sizeof(value) == sizeof(VALUE));

/// @brief Keeps a temporary Ruby value visible to the conservative GC.
/// @code rbxx::gc_guard(value); @endcode
inline void gc_guard(value guarded) noexcept {
  VALUE raw = guarded.raw();
  RB_GC_GUARD(raw);
}

} // namespace rbxx
