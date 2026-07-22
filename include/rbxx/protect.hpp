#pragma once

#include <rbxx/exception.hpp>

#include <exception>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace rbxx {
namespace detail {

template <typename Function, typename... Args> struct protect_payload {
  using function_type = std::decay_t<Function>;
  using result_type = std::invoke_result_t<function_type&, std::decay_t<Args>&...>;
  using stored_result = std::conditional_t<std::is_void_v<result_type>, bool, result_type>;

  function_type function;
  std::tuple<std::decay_t<Args>...> args;
  std::optional<stored_result> result;
  std::exception_ptr cpp_exception;
};

template <typename Payload> VALUE protect_trampoline(VALUE opaque) noexcept {
  auto* payload = reinterpret_cast<Payload*>(opaque);
  try {
    if constexpr (std::is_void_v<typename Payload::result_type>) {
      std::apply(payload->function, payload->args);
      payload->result.emplace(true);
    } else {
      payload->result.emplace(std::apply(payload->function, payload->args));
    }
  } catch (...) {
    payload->cpp_exception = std::current_exception();
  }
  return Qnil;
}

} // namespace detail

/// @brief Invokes a Ruby C API operation through rb_protect and converts longjmp to ruby_error.
/// @code VALUE string = rbxx::protect(rb_utf8_str_new_cstr, "safe"); @endcode
template <typename Function, typename... Args> auto protect(Function&& function, Args&&... args) {
  using payload_type = detail::protect_payload<Function, Args...>;
  static_assert(!std::is_reference_v<typename payload_type::result_type>,
                "rbxx::protect does not support reference return types");

  payload_type payload{std::forward<Function>(function),
                       std::tuple<std::decay_t<Args>...>{std::forward<Args>(args)...}, std::nullopt,
                       nullptr};
  int state = 0;
  rb_protect(detail::protect_trampoline<payload_type>, reinterpret_cast<VALUE>(&payload), &state);

  if (state != 0) {
    VALUE exception = rb_errinfo();
    rb_set_errinfo(Qnil);
    throw ruby_error(exception);
  }
  if (payload.cpp_exception) {
    std::rethrow_exception(payload.cpp_exception);
  }

  if constexpr (std::is_void_v<typename payload_type::result_type>) {
    return;
  } else {
    return std::move(*payload.result);
  }
}

} // namespace rbxx
