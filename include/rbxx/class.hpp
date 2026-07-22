#pragma once

#include <rbxx/module.hpp>

#include <iterator>
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
  explicit constructor_function(std::vector<argument_spec> specs = {})
      : parser_(std::move(specs)) {}

  [[nodiscard]] value invoke(int argc, const VALUE* argv, value self) override {
    parsed_arguments parsed = parser_.parse(argc, argv);
    auto converted = load(parsed, std::index_sequence_for<Args...>{});
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
    return parser_.configured() ? argc <= static_cast<int>(sizeof...(Args) + 1U)
                                : argc == static_cast<int>(sizeof...(Args));
  }

  [[nodiscard]] int match_score(int argc, const VALUE* argv) const noexcept override {
    if (parser_.configured()) {
      return accepts_arity(argc) ? 0 : -1;
    }
    using tuple = std::tuple<Args...>;
    return tuple_match_score<tuple>(argc, argv, std::index_sequence_for<Args...>{});
  }

  [[nodiscard]] std::size_t declared_arity() const noexcept override { return sizeof...(Args); }

private:
  template <std::size_t... Index>
  static auto load(const parsed_arguments& parsed, std::index_sequence<Index...>) {
    return std::tuple<decltype(load_parsed_argument<Args>(parsed, Index))...>{
        load_parsed_argument<Args>(parsed, Index)...};
  }

  argument_parser<std::tuple<Args...>> parser_;
};

template <typename Bound, typename Method> class member_function final : public native_function {
public:
  member_function(Method method, policy::kind selected, std::vector<argument_spec> specs = {},
                  std::vector<keep_alive_spec> keep_alive = {})
      : method_(method), policy_(selected), parser_(std::move(specs)),
        keep_alive_(std::move(keep_alive)) {}

  [[nodiscard]] value invoke(int argc, const VALUE* argv, value self) override {
    using traits = member_method_traits<Method>;
    parsed_arguments parsed = parser_.parse(argc, argv);
    Bound& native = load_registered<Bound>(self);
    value result = invoke_native(native, parsed, std::make_index_sequence<traits::arity>{});
    apply_keep_alive(keep_alive_, result, self, argc, argv);
    return result;
  }

  [[nodiscard]] std::string signature() const override { return std::string(type_name<Method>()); }

  [[nodiscard]] bool accepts_arity(int argc) const noexcept override {
    constexpr auto arity = member_method_traits<Method>::arity;
    return parser_.configured() ? argc <= static_cast<int>(arity + 1U)
                                : argc == static_cast<int>(arity);
  }

  [[nodiscard]] int match_score(int argc, const VALUE* argv) const noexcept override {
    using traits = member_method_traits<Method>;
    if (parser_.configured()) {
      return accepts_arity(argc) ? 0 : -1;
    }
    return tuple_match_score<typename traits::args_tuple>(
        argc, argv, std::make_index_sequence<traits::arity>{});
  }

  [[nodiscard]] std::size_t declared_arity() const noexcept override {
    return member_method_traits<Method>::arity;
  }

private:
  template <std::size_t... Index>
  [[nodiscard]] value invoke_native(Bound& self, const parsed_arguments& parsed,
                                    std::index_sequence<Index...>) {
    using traits = member_method_traits<Method>;
    using args = typename traits::args_tuple;
    auto converted = std::tuple<decltype(load_parsed_argument<std::tuple_element_t<Index, args>>(
        parsed, Index))...>{
        load_parsed_argument<std::tuple_element_t<Index, args>>(parsed, Index)...};
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
  argument_parser<typename member_method_traits<Method>::args_tuple> parser_;
  std::vector<keep_alive_spec> keep_alive_;
};

template <typename Tuple, std::size_t... Index>
consteval bool self_arguments_convertible(std::index_sequence<Index...>) {
  return (from_ruby_convertible<std::tuple_element_t<Index + 1U, Tuple>> && ...);
}

template <typename Tuple, std::size_t... Index>
auto tuple_tail_type(std::index_sequence<Index...>)
    -> std::tuple<std::tuple_element_t<Index + 1U, Tuple>...>;

template <typename Tuple>
using tuple_tail_t =
    decltype(tuple_tail_type<Tuple>(std::make_index_sequence<std::tuple_size_v<Tuple> - 1U>{}));

template <typename Bound, typename Function> class self_function final : public native_function {
public:
  using traits = resolved_function_traits<Function>;
  using args = typename traits::args_tuple;
  static constexpr std::size_t ruby_arity = traits::arity - 1U;

  self_function(Function function, policy::kind selected, std::vector<argument_spec> specs = {},
                std::vector<keep_alive_spec> keep_alive = {})
      : function_(std::move(function)), policy_(selected), parser_(std::move(specs)),
        keep_alive_(std::move(keep_alive)) {
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
    parsed_arguments parsed = parser_.parse(argc, argv);
    Bound& native = load_registered<Bound>(self);
    value result = invoke_native(native, parsed, std::make_index_sequence<ruby_arity>{});
    apply_keep_alive(keep_alive_, result, self, argc, argv);
    return result;
  }

  [[nodiscard]] std::string signature() const override {
    return std::string(type_name<Function>());
  }
  [[nodiscard]] bool accepts_arity(int argc) const noexcept override {
    return parser_.configured() ? argc <= static_cast<int>(ruby_arity + 1U)
                                : argc == static_cast<int>(ruby_arity);
  }
  [[nodiscard]] int match_score(int argc, const VALUE* argv) const noexcept override {
    if (parser_.configured()) {
      return accepts_arity(argc) ? 0 : -1;
    }
    using ruby_args = tuple_tail_t<args>;
    return tuple_match_score<ruby_args>(argc, argv, std::make_index_sequence<ruby_arity>{});
  }
  [[nodiscard]] std::size_t declared_arity() const noexcept override { return ruby_arity; }

private:
  template <std::size_t... Index>
  [[nodiscard]] value invoke_native(Bound& self, const parsed_arguments& parsed,
                                    std::index_sequence<Index...>) {
    auto converted =
        std::tuple<decltype(load_parsed_argument<std::tuple_element_t<Index + 1U, args>>(
            parsed, Index))...>{
            load_parsed_argument<std::tuple_element_t<Index + 1U, args>>(parsed, Index)...};
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
  argument_parser<tuple_tail_t<args>> parser_;
  std::vector<keep_alive_spec> keep_alive_;
};

template <typename Bound, auto Begin, auto End>
class iterable_function final : public native_function {
public:
  [[nodiscard]] value invoke(int, const VALUE*, value self) override {
    if (rb_block_given_p() == 0) {
      VALUE enumerator = protect([self] {
        return rb_enumeratorize_with_size(self.raw(), ID2SYM(rb_intern("each")), 0, nullptr,
                                          size_callback);
      });
      return value{enumerator};
    }

    Bound& native = load_registered<Bound>(self);
    auto iterator = std::invoke(Begin, native);
    const auto finish = std::invoke(End, native);
    for (; iterator != finish; ++iterator) {
      value item = to_ruby(*iterator);
      protect(rb_yield, item.raw());
    }
    return self;
  }

  [[nodiscard]] std::string signature() const override { return "each()"; }
  [[nodiscard]] bool accepts_arity(int argc) const noexcept override { return argc == 0; }
  [[nodiscard]] int match_score(int argc, const VALUE*) const noexcept override {
    return argc == 0 ? 0 : -1;
  }
  [[nodiscard]] std::size_t declared_arity() const noexcept override { return 0U; }

private:
  static VALUE size_callback(VALUE self, VALUE, VALUE) {
    VALUE result = Qnil;
    VALUE pending = Qnil;
    try {
      Bound& native = load_registered<Bound>(value{self});
      auto first = std::invoke(Begin, native);
      auto last = std::invoke(End, native);
      result = to_ruby(std::distance(first, last)).raw();
    } catch (...) {
      pending = translate_current_exception();
    }
    if (!NIL_P(pending)) {
      rb_exc_raise(pending);
    }
    return result;
  }
};

} // namespace detail

/// @brief Fluent binding handle for a C++ class exposed as Ruby TypedData.
/// @code binding.def(rbxx::init<int>()).def("value", &Counter::value); @endcode
template <typename T> class class_ {
public:
  explicit class_(value ruby_class) noexcept : ruby_class_(ruby_class) {}

  [[nodiscard]] value get() const noexcept { return ruby_class_; }

  template <typename... Args, typename... Specs>
    requires((std::is_convertible_v<Specs, argument_spec>) && ...)
  class_& def(init_tag<Args...>, Specs&&... specs) {
    static_assert(sizeof...(Specs) == 0U ||
                      sizeof...(Specs) == detail::normal_argument_count<std::tuple<Args...>>(),
                  "rbxx: argument annotation count must match constructor parameters");
    static_assert((from_ruby_convertible<Args> && ...),
                  "rbxx: constructor argument type has no type_caster");
    ID method = protect(rb_intern, "initialize");
    const bool first = detail::method_registry::instance().add(
        ruby_class_.raw(), method,
        std::make_unique<detail::constructor_function<T, Args...>>(
            detail::make_argument_specs(std::forward<Specs>(specs)...)));
    if (first) {
      detail::define_instance_trampoline(ruby_class_.raw(), "initialize");
    }
    return *this;
  }

  template <typename Function, typename... Specs>
    requires((std::is_convertible_v<Specs, argument_spec>) && ...)
  class_& def(const char* name, Function&& function, Specs&&... specs) {
    return bind_function(name, std::forward<Function>(function), policy::automatic,
                         detail::make_argument_specs(std::forward<Specs>(specs)...));
  }

  template <typename Function, typename... Specs>
    requires((std::is_convertible_v<Specs, argument_spec>) && ...)
  class_& def(const char* name, Function&& function, policy::return_value_policy selected,
              Specs&&... specs) {
    return bind_function(name, std::forward<Function>(function), selected,
                         detail::make_argument_specs(std::forward<Specs>(specs)...));
  }

  template <typename Function, std::size_t Nurse, std::size_t Patient, typename... Specs>
    requires((std::is_convertible_v<Specs, argument_spec>) && ...)
  class_& def(const char* name, Function&& function, policy::return_value_policy selected,
              keep_alive<Nurse, Patient> lifetime, Specs&&... specs) {
    return bind_function(name, std::forward<Function>(function), selected,
                         detail::make_argument_specs(std::forward<Specs>(specs)...),
                         {detail::make_keep_alive_spec(lifetime)});
  }

  template <typename Function, std::size_t Nurse, std::size_t Patient, typename... Specs>
    requires((std::is_convertible_v<Specs, argument_spec>) && ...)
  class_& def(const char* name, Function&& function, keep_alive<Nurse, Patient> lifetime,
              Specs&&... specs) {
    return bind_function(name, std::forward<Function>(function), policy::automatic,
                         detail::make_argument_specs(std::forward<Specs>(specs)...),
                         {detail::make_keep_alive_spec(lifetime)});
  }

  template <typename Function, typename... Specs>
    requires((std::is_convertible_v<Specs, argument_spec>) && ...)
  class_& def(op::name operation, Function&& function, Specs&&... specs) {
    class_& result =
        def(operation.ruby_name, std::forward<Function>(function), std::forward<Specs>(specs)...);
    include_comparable(operation);
    return result;
  }

  template <typename Function, typename... Specs>
    requires((std::is_convertible_v<Specs, argument_spec>) && ...)
  class_& def(op::name operation, Function&& function, policy::return_value_policy selected,
              Specs&&... specs) {
    class_& result = def(operation.ruby_name, std::forward<Function>(function), selected,
                         std::forward<Specs>(specs)...);
    include_comparable(operation);
    return result;
  }

  template <typename Function, typename... Specs>
  class_& def_static(const char* name, Function&& function, Specs&&... specs) {
    detail::register_static_function(ruby_class_.raw(), name, std::forward<Function>(function),
                                     detail::make_argument_specs(std::forward<Specs>(specs)...));
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

  /// @brief Defines #each, includes Enumerable, and returns sized enumerators without a block.
  template <auto Begin, auto End> class_& def_iterable() {
    using item_reference = decltype(*std::invoke(Begin, std::declval<T&>()));
    static_assert(to_ruby_convertible<item_reference>,
                  "rbxx: iterable item type has no type_caster");
    ID method = protect(rb_intern, "each");
    const bool first = detail::method_registry::instance().add(
        ruby_class_.raw(), method, std::make_unique<detail::iterable_function<T, Begin, End>>());
    if (first) {
      detail::define_instance_trampoline(ruby_class_.raw(), "each");
    }
    protect(rb_include_module, ruby_class_.raw(), rb_mEnumerable);
    return *this;
  }

private:
  void include_comparable(op::name operation) {
    if (operation.include_comparable) {
      protect(rb_include_module, ruby_class_.raw(), rb_mComparable);
    }
  }

  template <typename Function>
  class_& bind_function(const char* name, Function&& function, policy::return_value_policy selected,
                        std::vector<argument_spec> specs,
                        std::vector<detail::keep_alive_spec> keep_alive = {}) {
    using stored_function = std::decay_t<Function>;
    ID method = protect(rb_intern, name);
    if constexpr (std::is_member_function_pointer_v<stored_function>) {
      using traits = detail::member_method_traits<stored_function>;
      static_assert(detail::arguments_convertible<typename traits::args_tuple>(
                        std::make_index_sequence<traits::arity>{}),
                    "rbxx: member function argument type has no type_caster");
      const bool first = detail::method_registry::instance().add(
          ruby_class_.raw(), method,
          std::make_unique<detail::member_function<T, stored_function>>(
              function, selected.value, std::move(specs), std::move(keep_alive)));
      if (first) {
        detail::define_instance_trampoline(ruby_class_.raw(), name);
      }
    } else {
      static_assert(detail::function_signature<stored_function>,
                    "rbxx: instance callable signature cannot be determined");
      const bool first = detail::method_registry::instance().add(
          ruby_class_.raw(), method,
          std::make_unique<detail::self_function<T, stored_function>>(
              std::forward<Function>(function), selected.value, std::move(specs),
              std::move(keep_alive)));
      if (first) {
        detail::define_instance_trampoline(ruby_class_.raw(), name);
      }
    }
    return *this;
  }
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
