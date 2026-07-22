#pragma once

#include <rbxx/module.hpp>

#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace rbxx {

/// @brief Constructor signature marker used by class_::def.
/// @code binding.def(rbxx::init<int>()); @endcode
template <typename... Args> struct init_tag {};

/// @brief Creates a constructor signature marker.
template <typename... Args> constexpr init_tag<Args...> init() noexcept { return {}; }

namespace detail {

template <typename Method> struct member_method_traits;

template <typename Class, typename Return, typename... Args>
struct member_method_traits<Return (Class::*)(Args...)> : function_traits<Return(Args...)> {
  using class_type = Class;
};

template <typename Class, typename Return, typename... Args>
struct member_method_traits<Return (Class::*)(Args...) const> : function_traits<Return(Args...)> {
  using class_type = Class;
};

template <typename Class, typename Return, typename... Args>
struct member_method_traits<Return (Class::*)(Args...) noexcept>
    : function_traits<Return(Args...)> {
  using class_type = Class;
};

template <typename Class, typename Return, typename... Args>
struct member_method_traits<Return (Class::*)(Args...) const noexcept>
    : function_traits<Return(Args...)> {
  using class_type = Class;
};

inline void define_instance_trampoline(VALUE klass, const char* name) {
  protect([klass, name] { rb_define_method(klass, name, function_trampoline, -1); });
}

template <typename T, typename... Args> class constructor_function final : public native_function {
public:
  [[nodiscard]] value invoke(int argc, const VALUE* argv, value self) override {
    if (!accepts_arity(argc)) {
      std::ostringstream message;
      message << "rbxx: argument count mismatch for constructor; expected " << sizeof...(Args)
              << ", actual " << argc;
      throw ruby_error(make_exception(rb_eArgError, message.str().c_str()));
    }

    auto converted = load(argv, std::index_sequence_for<Args...>{});
    auto& wrapper = exact_wrapper<T>(self);
    if (wrapper.pointer != nullptr) {
      throw ruby_error(make_exception(rb_eRuntimeError, "rbxx: C++ object is already initialized"));
    }
    wrapper.pointer = std::apply(
        [](auto&&... arguments) { return new T(std::forward<decltype(arguments)>(arguments)...); },
        converted);
    wrapper.mode = ownership::owned;
    return value{Qnil};
  }

  [[nodiscard]] std::string signature() const override {
    return "init<" + std::string(type_name<T>()) + ">";
  }

  [[nodiscard]] bool accepts_arity(int argc) const noexcept override {
    return argc == static_cast<int>(sizeof...(Args));
  }

private:
  template <std::size_t... Index>
  static auto load(const VALUE* argv, std::index_sequence<Index...>) {
    return std::tuple<decltype(load_argument<Args>(value{argv[Index]}))...>{
        load_argument<Args>(value{argv[Index]})...};
  }
};

template <typename Bound, typename Method> class member_function final : public native_function {
public:
  member_function(Method method, policy::kind selected) : method_(method), policy_(selected) {}

  [[nodiscard]] value invoke(int argc, const VALUE* argv, value self) override {
    using traits = member_method_traits<Method>;
    if (!accepts_arity(argc)) {
      std::ostringstream message;
      message << "rbxx: argument count mismatch for " << signature() << "; expected "
              << traits::arity << ", actual " << argc;
      throw ruby_error(make_exception(rb_eArgError, message.str().c_str()));
    }
    Bound& native = load_registered<Bound>(self);
    return invoke_native(native, argv, std::make_index_sequence<traits::arity>{});
  }

  [[nodiscard]] std::string signature() const override { return std::string(type_name<Method>()); }

  [[nodiscard]] bool accepts_arity(int argc) const noexcept override {
    return argc == static_cast<int>(member_method_traits<Method>::arity);
  }

private:
  template <std::size_t... Index>
  [[nodiscard]] value invoke_native(Bound& self, const VALUE* argv, std::index_sequence<Index...>) {
    using traits = member_method_traits<Method>;
    using args = typename traits::args_tuple;
    auto converted = std::tuple<decltype(load_argument<std::tuple_element_t<Index, args>>(value{
        argv[Index]}))...>{load_argument<std::tuple_element_t<Index, args>>(value{argv[Index]})...};
    if constexpr (std::is_void_v<typename traits::return_type>) {
      std::apply([&](auto&&... values) { std::invoke(method_, self, values...); }, converted);
      return value{Qnil};
    } else {
      decltype(auto) result = std::apply(
          [&](auto&&... values) -> decltype(auto) { return std::invoke(method_, self, values...); },
          converted);
      return dump_result(std::forward<decltype(result)>(result), policy_);
    }
  }

  Method method_;
  policy::kind policy_;
};

template <typename Tuple, std::size_t... Index>
consteval bool self_arguments_convertible(std::index_sequence<Index...>) {
  return (from_ruby_convertible<std::tuple_element_t<Index + 1U, Tuple>> && ...);
}

template <typename Bound, typename Function> class self_function final : public native_function {
public:
  using traits = resolved_function_traits<Function>;
  using args = typename traits::args_tuple;
  static constexpr std::size_t ruby_arity = traits::arity - 1U;

  self_function(Function function, policy::kind selected)
      : function_(std::move(function)), policy_(selected) {
    static_assert(traits::arity > 0,
                  "rbxx: instance lambda must accept self as its first argument");
    using self_argument = std::tuple_element_t<0, args>;
    static_assert(std::is_lvalue_reference_v<self_argument> &&
                      std::is_same_v<std::remove_cvref_t<self_argument>, Bound>,
                  "rbxx: instance lambda first argument must be T& or const T&");
    static_assert(self_arguments_convertible<args>(std::make_index_sequence<ruby_arity>{}),
                  "rbxx: instance lambda argument type has no type_caster");
  }

  [[nodiscard]] value invoke(int argc, const VALUE* argv, value self) override {
    if (!accepts_arity(argc)) {
      std::ostringstream message;
      message << "rbxx: argument count mismatch for " << signature() << "; expected " << ruby_arity
              << ", actual " << argc;
      throw ruby_error(make_exception(rb_eArgError, message.str().c_str()));
    }
    Bound& native = load_registered<Bound>(self);
    return invoke_native(native, argv, std::make_index_sequence<ruby_arity>{});
  }

  [[nodiscard]] std::string signature() const override {
    return std::string(type_name<Function>());
  }
  [[nodiscard]] bool accepts_arity(int argc) const noexcept override {
    return argc == static_cast<int>(ruby_arity);
  }

private:
  template <std::size_t... Index>
  [[nodiscard]] value invoke_native(Bound& self, const VALUE* argv, std::index_sequence<Index...>) {
    auto converted = std::tuple<decltype(load_argument<std::tuple_element_t<Index + 1U, args>>(
        value{argv[Index]}))...>{
        load_argument<std::tuple_element_t<Index + 1U, args>>(value{argv[Index]})...};
    if constexpr (std::is_void_v<typename traits::return_type>) {
      std::apply([&](auto&&... values) { std::invoke(function_, self, values...); }, converted);
      return value{Qnil};
    } else {
      decltype(auto) result = std::apply(
          [&](auto&&... values) -> decltype(auto) {
            return std::invoke(function_, self, values...);
          },
          converted);
      return dump_result(std::forward<decltype(result)>(result), policy_);
    }
  }

  Function function_;
  policy::kind policy_;
};

} // namespace detail

/// @brief Fluent binding handle for a C++ class exposed as Ruby TypedData.
/// @code binding.def(rbxx::init<int>()).def("value", &Counter::value); @endcode
template <typename T> class class_ {
public:
  explicit class_(value ruby_class) noexcept : ruby_class_(ruby_class) {}

  [[nodiscard]] value get() const noexcept { return ruby_class_; }

  template <typename... Args> class_& def(init_tag<Args...>) {
    static_assert((from_ruby_convertible<Args> && ...),
                  "rbxx: constructor argument type has no type_caster");
    ID method = protect(rb_intern, "initialize");
    const bool first = detail::method_registry::instance().add(
        ruby_class_.raw(), method, std::make_unique<detail::constructor_function<T, Args...>>());
    if (first) {
      detail::define_instance_trampoline(ruby_class_.raw(), "initialize");
    }
    return *this;
  }

  template <typename Function>
  class_& def(const char* name, Function&& function,
              policy::return_value_policy selected = policy::automatic) {
    using stored_function = std::decay_t<Function>;
    ID method = protect(rb_intern, name);
    if constexpr (std::is_member_function_pointer_v<stored_function>) {
      using traits = detail::member_method_traits<stored_function>;
      static_assert(detail::arguments_convertible<typename traits::args_tuple>(
                        std::make_index_sequence<traits::arity>{}),
                    "rbxx: member function argument type has no type_caster");
      const bool first = detail::method_registry::instance().add(
          ruby_class_.raw(), method,
          std::make_unique<detail::member_function<T, stored_function>>(function, selected.value));
      if (first) {
        detail::define_instance_trampoline(ruby_class_.raw(), name);
      }
    } else {
      static_assert(detail::function_signature<stored_function>,
                    "rbxx: instance callable signature cannot be determined");
      const bool first = detail::method_registry::instance().add(
          ruby_class_.raw(), method,
          std::make_unique<detail::self_function<T, stored_function>>(
              std::forward<Function>(function), selected.value));
      if (first) {
        detail::define_instance_trampoline(ruby_class_.raw(), name);
      }
    }
    return *this;
  }

  template <typename Function> class_& def_static(const char* name, Function&& function) {
    detail::register_static_function(ruby_class_.raw(), name, std::forward<Function>(function));
    return *this;
  }

  template <typename Member> class_& def_attr_reader(const char* name, Member T::* member) {
    return def(name, [member](const T& self) -> const Member& { return self.*member; });
  }

  template <typename Member> class_& def_attr_writer(const char* name, Member T::* member) {
    std::string writer = std::string(name) + "=";
    return def(writer.c_str(), [member](T& self, Member updated) {
      self.*member = std::move(updated);
      return self.*member;
    });
  }

  template <typename Member> class_& def_attr_accessor(const char* name, Member T::* member) {
    def_attr_reader(name, member);
    return def_attr_writer(name, member);
  }

private:
  value ruby_class_;
};

template <typename T, typename Base>
class_<T> module::def_class(const char* name, std::source_location location) {
  VALUE superclass = rb_cObject;
  if constexpr (!std::is_void_v<Base>) {
    static_assert(std::is_base_of_v<Base, T>, "rbxx: Base must be a C++ base class of T");
    superclass = detail::registered_class<Base>().ruby_class;
  }
  VALUE klass = protect(rb_define_class_under, wrapped_.raw(), name, superclass);
  detail::register_data_class<T, Base>(klass, location);
  protect([klass] { rb_define_alloc_func(klass, detail::allocate_data_object<T>); });
  return class_<T>{value{klass}};
}

} // namespace rbxx
