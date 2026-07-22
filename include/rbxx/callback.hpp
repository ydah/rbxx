#pragma once

#include <rbxx/data_object.hpp>

#include <array>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace rbxx {

namespace detail {

template <typename Return, typename... Args>
struct is_non_bindable_class<std::function<Return(Args...)>> : std::true_type {};

inline void require_callback_gvl() {
#if defined(RBXX_DEBUG)
  if (ruby_thread_has_gvl_p() == 0) {
    throw std::runtime_error(
        "rbxx: Ruby callback invoked without the GVL; reacquire it before calling the Proc");
  }
#endif
}

} // namespace detail

/// @brief Converts a Ruby Proc into a GC-pinned C++ std::function.
template <typename Return, typename... Args> struct type_caster<std::function<Return(Args...)>> {
  static constexpr std::string_view name = "Proc";

  static std::function<Return(Args...)> load(value input) {
    if (!matches(input)) {
      detail::throw_type_error("Proc", input);
    }
    object callable{input};
    return [callable = std::move(callable)](Args... args) -> Return {
      detail::require_callback_gvl();
      std::array<VALUE, sizeof...(Args)> converted{to_ruby(std::forward<Args>(args)).raw()...};
      VALUE result = protect(rb_funcallv, callable.raw(), rb_intern("call"),
                             static_cast<int>(converted.size()), converted.data());
      if constexpr (std::is_void_v<Return>) {
        return;
      } else {
        return from_ruby<Return>(value{result});
      }
    };
  }

  static bool matches(value input) noexcept { return RTEST(rb_obj_is_proc(input.raw())); }
};

} // namespace rbxx
