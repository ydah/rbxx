#pragma once

#include <rbxx/function.hpp>

#include <exception>
#include <functional>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace rbxx {
namespace detail {

struct default_unblock_function {};

template <typename T> struct is_std_function : std::false_type {};
template <typename Return, typename... Args>
struct is_std_function<std::function<Return(Args...)>> : std::true_type {};

template <typename T>
inline constexpr bool gvl_independent_v = !std::is_same_v<std::remove_cvref_t<T>, value> &&
                                          !std::is_same_v<std::remove_cvref_t<T>, object> &&
                                          !std::is_same_v<std::remove_cvref_t<T>, block> &&
                                          !std::is_same_v<std::remove_cvref_t<T>, optional_block> &&
                                          !std::is_same_v<std::remove_cvref_t<T>, args> &&
                                          !is_std_function<std::remove_cvref_t<T>>::value;

template <typename Function, typename Unblock, typename Return, typename Tuple>
class nogvl_adapter_impl;

template <typename Function, typename Unblock, typename Return, typename... Args>
class nogvl_adapter_impl<Function, Unblock, Return, std::tuple<Args...>> {
public:
  explicit nogvl_adapter_impl(Function function) : function_(std::move(function)) {
    validate_signature();
  }

  nogvl_adapter_impl(Function function, Unblock unblock)
      : function_(std::move(function)), unblock_(std::move(unblock)) {
    validate_signature();
    static_assert(std::is_nothrow_invocable_v<Unblock&>,
                  "rbxx: nogvl interrupt function must be noexcept");
  }

  Return operator()(Args... args) {
    payload work{this, std::tuple<Args...>{std::forward<Args>(args)...}, std::nullopt, nullptr};
    if constexpr (std::is_same_v<Unblock, default_unblock_function>) {
      protect([&work] { rb_thread_call_without_gvl(run, &work, RUBY_UBF_IO, nullptr); });
    } else {
      protect([&work, this] {
        rb_thread_call_without_gvl(run, &work, interrupt, std::addressof(unblock_));
      });
    }
    if (work.exception) {
      std::rethrow_exception(work.exception);
    }
    if constexpr (!std::is_void_v<Return>) {
      if (!work.result) {
        throw std::runtime_error("rbxx: nogvl function was interrupted before producing a result");
      }
      return std::move(*work.result);
    }
  }

private:
  using stored_result = std::conditional_t<std::is_void_v<Return>, bool, Return>;

  struct payload {
    nogvl_adapter_impl* adapter;
    std::tuple<Args...> arguments;
    std::optional<stored_result> result;
    std::exception_ptr exception;
  };

  static consteval void validate_signature() {
    static_assert(!std::is_reference_v<Return>,
                  "rbxx: nogvl return values must be owned C++ values");
    static_assert(gvl_independent_v<Return>,
                  "rbxx: nogvl return type must not access Ruby without the GVL");
    static_assert((gvl_independent_v<Args> && ...),
                  "rbxx: nogvl arguments must not access Ruby without the GVL");
  }

  static void* run(void* opaque) noexcept {
    auto* work = static_cast<payload*>(opaque);
    try {
      if constexpr (std::is_void_v<Return>) {
        std::apply(
            [&](auto&&... values) {
              std::invoke(work->adapter->function_, std::forward<Args>(values)...);
            },
            work->arguments);
        work->result.emplace(true);
      } else {
        work->result.emplace(std::apply(
            [&](auto&&... values) -> Return {
              return std::invoke(work->adapter->function_, std::forward<Args>(values)...);
            },
            work->arguments));
      }
    } catch (...) {
      work->exception = std::current_exception();
    }
    return nullptr;
  }

  static void interrupt(void* opaque) noexcept { std::invoke(*static_cast<Unblock*>(opaque)); }

  Function function_;
  [[no_unique_address]] Unblock unblock_{};
};

template <typename Function, typename Unblock = default_unblock_function>
using nogvl_adapter =
    nogvl_adapter_impl<std::decay_t<Function>, std::decay_t<Unblock>,
                       typename resolved_function_traits<std::decay_t<Function>>::return_type,
                       typename resolved_function_traits<std::decay_t<Function>>::args_tuple>;

} // namespace detail

/// @brief Wraps a pure C++ callable so its execution occurs without the Ruby GVL.
template <typename Function> [[nodiscard]] auto nogvl(Function&& function) {
  using stored_function = std::decay_t<Function>;
  static_assert(detail::function_signature<stored_function>,
                "rbxx: nogvl requires a callable with a concrete signature");
  static_assert(!std::is_member_function_pointer_v<stored_function>,
                "rbxx: nogvl expects a free function or callable object");
  return detail::nogvl_adapter<stored_function>{std::forward<Function>(function)};
}

/// @brief Wraps a pure C++ callable with a noexcept interruption hook.
template <typename Function, typename Unblock>
[[nodiscard]] auto nogvl_interruptible(Function&& function, Unblock&& unblock) {
  using stored_function = std::decay_t<Function>;
  using stored_unblock = std::decay_t<Unblock>;
  static_assert(detail::function_signature<stored_function>,
                "rbxx: nogvl_interruptible requires a callable with a concrete signature");
  static_assert(!std::is_member_function_pointer_v<stored_function>,
                "rbxx: nogvl_interruptible expects a free function or callable object");
  return detail::nogvl_adapter<stored_function, stored_unblock>{std::forward<Function>(function),
                                                                std::forward<Unblock>(unblock)};
}

} // namespace rbxx
