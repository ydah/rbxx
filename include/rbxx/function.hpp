#pragma once

#include <rbxx/data_object.hpp>
#include <rbxx/policies.hpp>

#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace rbxx::detail {

template <typename T> struct function_traits;

template <typename Return, typename... Args> struct function_traits<Return(Args...)> {
  using return_type = Return;
  using args_tuple = std::tuple<Args...>;
  static constexpr std::size_t arity = sizeof...(Args);
};

template <typename Return, typename... Args>
struct function_traits<Return (*)(Args...)> : function_traits<Return(Args...)> {};

template <typename Return, typename... Args>
struct function_traits<Return (*)(Args...) noexcept> : function_traits<Return(Args...)> {};

template <typename Return, typename... Args>
struct function_traits<std::function<Return(Args...)>> : function_traits<Return(Args...)> {};

template <typename Class, typename Return, typename... Args>
struct function_traits<Return (Class::*)(Args...)> : function_traits<Return(Args...)> {};

template <typename Class, typename Return, typename... Args>
struct function_traits<Return (Class::*)(Args...) const> : function_traits<Return(Args...)> {};

template <typename Class, typename Return, typename... Args>
struct function_traits<Return (Class::*)(Args...) noexcept> : function_traits<Return(Args...)> {};

template <typename Class, typename Return, typename... Args>
struct function_traits<Return (Class::*)(Args...) const noexcept>
    : function_traits<Return(Args...)> {};

template <typename T, typename = void> struct resolved_function_traits {};

template <typename T>
struct resolved_function_traits<T, std::void_t<decltype(&std::remove_reference_t<T>::operator())>>
    : function_traits<decltype(&std::remove_reference_t<T>::operator())> {};

template <typename T>
  requires std::is_function_v<std::remove_pointer_t<std::decay_t<T>>>
struct resolved_function_traits<T, void> : function_traits<std::decay_t<T>> {};

template <typename Return, typename... Args>
struct resolved_function_traits<std::function<Return(Args...)>, void>
    : function_traits<std::function<Return(Args...)>> {};

template <typename T>
concept function_signature = requires {
  typename resolved_function_traits<std::decay_t<T>>::return_type;
  typename resolved_function_traits<std::decay_t<T>>::args_tuple;
};

template <typename Tuple, std::size_t... Index>
consteval bool arguments_convertible(std::index_sequence<Index...>) {
  return (from_ruby_convertible<std::tuple_element_t<Index, Tuple>> && ...);
}

template <typename Function> consteval bool function_arguments_convertible() {
  using traits = resolved_function_traits<std::decay_t<Function>>;
  using args = typename traits::args_tuple;
  return arguments_convertible<args>(std::make_index_sequence<std::tuple_size_v<args>>{});
}

template <typename Function> consteval bool function_return_convertible() {
  using result = typename resolved_function_traits<std::decay_t<Function>>::return_type;
  return std::is_void_v<result> || to_ruby_convertible<result>;
}

template <typename Argument> auto load_argument(value input) {
  using loaded_type = decltype(from_ruby<Argument>(input));
  if constexpr (std::is_lvalue_reference_v<loaded_type>) {
    return std::ref(from_ruby<Argument>(input));
  } else {
    return from_ruby<Argument>(input);
  }
}

template <typename Result> value dump_result(Result&& result, policy::kind selected);

class native_function {
public:
  native_function() = default;
  native_function(const native_function&) = delete;
  native_function& operator=(const native_function&) = delete;
  virtual ~native_function() = default;

  [[nodiscard]] virtual value invoke(int argc, const VALUE* argv, value self) = 0;
  [[nodiscard]] virtual std::string signature() const = 0;
  [[nodiscard]] virtual bool accepts_arity(int argc) const noexcept = 0;
};

template <typename Function> class native_function_impl final : public native_function {
public:
  explicit native_function_impl(Function function) : function_(std::move(function)) {}

  [[nodiscard]] value invoke(int argc, const VALUE* argv, value) override {
    using traits = resolved_function_traits<Function>;
    constexpr auto expected = traits::arity;
    if (argc != static_cast<int>(expected)) {
      std::ostringstream message;
      message << "rbxx: argument count mismatch for " << signature() << "; expected " << expected
              << ", actual " << argc;
      throw ruby_error(make_exception(rb_eArgError, message.str().c_str()));
    }
    return invoke_with_args(argv, std::make_index_sequence<expected>{});
  }

  [[nodiscard]] std::string signature() const override {
    return std::string(type_name<Function>());
  }

  [[nodiscard]] bool accepts_arity(int argc) const noexcept override {
    return argc == static_cast<int>(resolved_function_traits<Function>::arity);
  }

private:
  template <std::size_t... Index>
  [[nodiscard]] value invoke_with_args(const VALUE* argv, std::index_sequence<Index...>) {
    using traits = resolved_function_traits<Function>;
    using args = typename traits::args_tuple;
    auto converted = std::tuple<decltype(load_argument<std::tuple_element_t<Index, args>>(value{
        argv[Index]}))...>{load_argument<std::tuple_element_t<Index, args>>(value{argv[Index]})...};

    if constexpr (std::is_void_v<typename traits::return_type>) {
      std::apply(function_, converted);
      return value{Qnil};
    } else {
      decltype(auto) result = std::apply(function_, converted);
      return dump_result(std::forward<decltype(result)>(result), policy::kind::automatic);
    }
  }

  Function function_;
};

template <typename Result> value dump_result(Result&& result, policy::kind selected) {
  using result_type = Result;
  using bare_type = std::remove_cvref_t<Result>;
  if constexpr (std::is_pointer_v<bare_type> && std::is_class_v<std::remove_pointer_t<bare_type>>) {
    using pointee = std::remove_pointer_t<bare_type>;
    if (result == nullptr) {
      return value{Qnil};
    }
    if (selected == policy::kind::copy) {
      return wrap_copy(*result);
    }
    if (selected == policy::kind::take) {
      return wrap_take(std::unique_ptr<pointee>{result});
    }
    if (selected == policy::kind::shared) {
      throw std::invalid_argument("rbxx: shared policy requires std::shared_ptr<T>");
    }
    return wrap_reference(result);
  } else if constexpr (std::is_lvalue_reference_v<result_type> && std::is_class_v<bare_type>) {
    if (selected == policy::kind::copy) {
      return wrap_copy(result);
    }
    if (selected == policy::kind::take || selected == policy::kind::shared) {
      throw std::invalid_argument("rbxx: take/shared policy cannot be used with a reference");
    }
    return wrap_reference(std::addressof(result));
  } else {
    return to_ruby(std::forward<Result>(result));
  }
}

struct method_key {
  VALUE owner;
  ID method;

  friend bool operator==(const method_key&, const method_key&) = default;
};

struct method_key_hash {
  [[nodiscard]] std::size_t operator()(const method_key& key) const noexcept {
    const auto owner = static_cast<std::size_t>(key.owner);
    const auto method = static_cast<std::size_t>(key.method);
    return owner ^ (method + 0x9e3779b9U + (owner << 6U) + (owner >> 2U));
  }
};

class method_registry {
public:
  static method_registry& instance() {
    static auto* registry = new method_registry();
    return *registry;
  }

  bool add(VALUE owner, ID method, std::unique_ptr<native_function> function) {
    std::lock_guard lock(write_mutex_);
    auto& overloads = functions_[method_key{owner, method}];
    const bool first = overloads.empty();
    overloads.push_back(std::move(function));
    return first;
  }

  using overloads = std::vector<std::unique_ptr<native_function>>;

  [[nodiscard]] const overloads* find(VALUE self, ID method) const noexcept {
    if (const auto* direct = find_exact(self, method)) {
      return direct;
    }

    VALUE klass = CLASS_OF(self);
    while (!NIL_P(klass)) {
      if (const auto* inherited = find_exact(klass, method)) {
        return inherited;
      }
      klass = rb_class_superclass(klass);
    }
    return find_exact(Qnil, method);
  }

private:
  [[nodiscard]] const overloads* find_exact(VALUE owner, ID method) const noexcept {
    auto found = functions_.find(method_key{owner, method});
    return found == functions_.end() ? nullptr : std::addressof(found->second);
  }

  std::mutex write_mutex_;
  std::unordered_map<method_key, overloads, method_key_hash> functions_;
};

inline VALUE function_trampoline(int argc, VALUE* argv, VALUE self) {
  VALUE pending = Qnil;
  VALUE result = Qnil;
  try {
    ID method = rb_frame_this_func();
    const auto* functions = method_registry::instance().find(self, method);
    if (functions == nullptr || functions->empty()) {
      throw std::runtime_error("rbxx: native function registry entry was not found");
    }
    native_function* selected = functions->front().get();
    for (const auto& candidate : *functions) {
      if (candidate->accepts_arity(argc)) {
        selected = candidate.get();
        break;
      }
    }
    result = selected->invoke(argc, argv, value{self}).raw();
  } catch (...) {
    pending = translate_current_exception();
  }
  if (!NIL_P(pending)) {
    rb_exc_raise(pending);
  }
  return result;
}

template <typename Function>
void register_function(VALUE owner, const char* name, Function&& function, bool global = false) {
  using stored_function = std::decay_t<Function>;
  if constexpr (!function_signature<stored_function>) {
    static_assert(
        function_signature<stored_function>,
        "rbxx: callable signature cannot be determined; use a non-generic lambda, function "
        "pointer, or std::function");
  } else if constexpr (!function_arguments_convertible<stored_function>()) {
    static_assert(
        function_arguments_convertible<stored_function>(),
        "rbxx: function argument type has no type_caster; bind the type with def_class<T>() "
        "or specialize rbxx::type_caster<T>");
  } else if constexpr (!function_return_convertible<stored_function>()) {
    static_assert(
        function_return_convertible<stored_function>(),
        "rbxx: function return type has no type_caster; bind the type with def_class<T>() "
        "or specialize rbxx::type_caster<T>");
  } else {
    ID method = protect(rb_intern, name);
    const bool first = method_registry::instance().add(
        global ? Qnil : owner, method,
        std::make_unique<native_function_impl<stored_function>>(std::forward<Function>(function)));
    if (first) {
      protect([owner, name, global] {
        if (global) {
          rb_define_global_function(name, function_trampoline, -1);
        } else {
          rb_define_module_function(owner, name, function_trampoline, -1);
        }
      });
    }
  }
}

template <typename Function>
void register_static_function(VALUE owner, const char* name, Function&& function) {
  using stored_function = std::decay_t<Function>;
  if constexpr (!function_signature<stored_function>) {
    static_assert(
        function_signature<stored_function>,
        "rbxx: callable signature cannot be determined; use a non-generic lambda, function "
        "pointer, or std::function");
  } else if constexpr (!function_arguments_convertible<stored_function>()) {
    static_assert(function_arguments_convertible<stored_function>(),
                  "rbxx: function argument type has no type_caster");
  } else if constexpr (!function_return_convertible<stored_function>()) {
    static_assert(function_return_convertible<stored_function>(),
                  "rbxx: function return type has no type_caster");
  } else {
    ID method = protect(rb_intern, name);
    const bool first = method_registry::instance().add(
        owner, method,
        std::make_unique<native_function_impl<stored_function>>(std::forward<Function>(function)));
    if (first) {
      protect([owner, name] { rb_define_singleton_method(owner, name, function_trampoline, -1); });
    }
  }
}

} // namespace rbxx::detail
