#pragma once

#include <rbxx/function.hpp>
#include <rbxx/operators.hpp>

#include <source_location>
#include <utility>

namespace rbxx {

template <typename T> class class_;

/// @brief A non-owning DSL handle for a Ruby Module.
/// @code rbxx::define_module("Demo").def("answer", [] { return 42; }); @endcode
class module {
public:
  /// @brief Wraps an existing Ruby module VALUE.
  explicit module(value wrapped) noexcept : wrapped_(wrapped) {}

  /// @brief Returns the wrapped Ruby module.
  [[nodiscard]] value get() const noexcept { return wrapped_; }

  /// @brief Defines a Ruby module function backed by a C++ callable.
  /// @code mod.def("sum", [](int a, int b) { return a + b; }); @endcode
  template <typename Function, typename... Specs>
  module& def(const char* name, Function&& function, Specs&&... specs) {
    using traits = detail::resolved_function_traits<std::decay_t<Function>>;
    if constexpr (detail::function_signature<std::decay_t<Function>>) {
      static_assert(sizeof...(Specs) == 0U ||
                        sizeof...(Specs) ==
                            detail::normal_argument_count<typename traits::args_tuple>(),
                    "rbxx: argument annotation count must match callable parameters");
    }
    detail::register_function(wrapped_.raw(), name, std::forward<Function>(function),
                              detail::make_argument_specs(std::forward<Specs>(specs)...));
    return *this;
  }

  template <typename Function, typename... Specs>
  module& def(op::name operation, Function&& function, Specs&&... specs) {
    return def(operation.ruby_name, std::forward<Function>(function),
               std::forward<Specs>(specs)...);
  }

  /// @brief Defines a Ruby class backed by CRuby TypedData.
  /// @code auto counter = mod.def_class<Counter>("Counter"); @endcode
  template <typename T, typename Base = void>
  class_<T> def_class(const char* name,
                      std::source_location location = std::source_location::current());

private:
  value wrapped_;
};

/// @brief Defines or reopens a top-level Ruby module.
/// @code rbxx::module demo = rbxx::define_module("Demo"); @endcode
inline module define_module(const char* name) {
  return module{value{protect(rb_define_module, name)}};
}

/// @brief Defines a top-level Ruby global function.
/// @code rbxx::define_global_function("native_sum", [](int a, int b) { return a + b; }); @endcode
template <typename Function, typename... Specs>
void define_global_function(const char* name, Function&& function, Specs&&... specs) {
  detail::register_function(Qnil, name, std::forward<Function>(function),
                            detail::make_argument_specs(std::forward<Specs>(specs)...), true);
}

} // namespace rbxx
