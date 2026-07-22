#pragma once

#include <rbxx/arg.hpp>
#include <rbxx/data_object.hpp>
#include <rbxx/policies.hpp>

#include <functional>
#include <limits>
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

struct keep_alive_spec {
  std::size_t nurse;
  std::size_t patient;
};

template <std::size_t Nurse, std::size_t Patient>
constexpr keep_alive_spec make_keep_alive_spec(keep_alive<Nurse, Patient>) noexcept {
  return {Nurse, Patient};
}

inline value keep_alive_value(std::size_t index, value result, value self, int argc,
                              const VALUE* argv) {
  if (index == 0U) {
    return result;
  }
  if (index == 1U) {
    return self;
  }
  const std::size_t argument = index - 2U;
  if (argument >= static_cast<std::size_t>(argc)) {
    throw std::out_of_range("rbxx: keep_alive index exceeds the Ruby argument count");
  }
  return value{argv[argument]};
}

inline void apply_keep_alive(const std::vector<keep_alive_spec>& policies, value result, value self,
                             int argc, const VALUE* argv) {
  for (const keep_alive_spec& selected : policies) {
    value nurse = keep_alive_value(selected.nurse, result, self, argc, argv);
    value patient = keep_alive_value(selected.patient, result, self, argc, argv);
    if (nurse.is_nil() || patient.is_nil()) {
      continue;
    }
    if (selected.nurse > static_cast<std::size_t>(std::numeric_limits<unsigned int>::max())) {
      throw std::out_of_range("rbxx: keep_alive nurse index is too large");
    }
    std::string name = "@__rbxx_keep_" + std::to_string(selected.nurse);
    protect([nurse, patient, &name] {
      ID id = rb_intern(name.c_str());
      VALUE storage = rb_ivar_get(nurse.raw(), id);
      if (NIL_P(storage)) {
        storage = rb_ary_new();
        rb_ivar_set(nurse.raw(), id, storage);
      } else if (!RB_TYPE_P(storage, T_ARRAY)) {
        rb_raise(rb_eTypeError, "rbxx: keep_alive storage was replaced with a non-Array");
      }
      rb_ary_push(storage, patient.raw());
    });
  }
}

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

template <typename Argument> int argument_match_score(value input) noexcept {
  using type = std::remove_cvref_t<Argument>;
  if constexpr (is_special_argument_v<type>) {
    return 0;
  } else if constexpr (std::is_integral_v<type> && !std::is_same_v<type, bool>) {
    return input.is_integer() ? 2 : 0;
  } else if constexpr (std::is_floating_point_v<type>) {
    return input.is_float() ? 2 : (input.is_integer() ? 1 : 0);
  } else if constexpr (std::is_same_v<type, std::string> ||
                       std::is_same_v<type, std::string_view> ||
                       std::is_same_v<type, const char*>) {
    return input.is_string() ? 2 : 0;
  } else if constexpr (std::is_same_v<type, value> || std::is_same_v<type, object>) {
    return 1;
  } else {
    return type_caster<type>::matches(input) ? 2 : 0;
  }
}

template <typename Tuple, std::size_t... Index>
int tuple_match_score(int argc, const VALUE* argv, std::index_sequence<Index...>) noexcept {
  if (argc != static_cast<int>(sizeof...(Index))) {
    return -1;
  }
  std::array<int, sizeof...(Index)> scores{
      argument_match_score<std::tuple_element_t<Index, Tuple>>(value{argv[Index]})...};
  int total = 0;
  for (int score : scores) {
    if (score == 0) {
      return -1;
    }
    total += score;
  }
  return total;
}

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
  [[nodiscard]] virtual int match_score(int argc, const VALUE* argv) const noexcept = 0;
  [[nodiscard]] virtual std::size_t declared_arity() const noexcept = 0;
};

template <typename Function> class native_function_impl final : public native_function {
public:
  explicit native_function_impl(Function function, std::vector<argument_spec> specs = {})
      : function_(std::move(function)), parser_(std::move(specs)) {}

  [[nodiscard]] value invoke(int argc, const VALUE* argv, value) override {
    using traits = resolved_function_traits<Function>;
    parsed_arguments parsed = parser_.parse(argc, argv);
    return invoke_with_args(parsed, std::make_index_sequence<traits::arity>{});
  }

  [[nodiscard]] std::string signature() const override {
    return std::string(type_name<Function>());
  }

  [[nodiscard]] bool accepts_arity(int argc) const noexcept override {
    using traits = resolved_function_traits<Function>;
    if (has_rest_args(std::make_index_sequence<traits::arity>{})) {
      return true;
    }
    if (parser_.configured() || has_special_args(std::make_index_sequence<traits::arity>{})) {
      return argc <= static_cast<int>(traits::arity + 1U);
    }
    return argc == static_cast<int>(traits::arity);
  }

  [[nodiscard]] int match_score(int argc, const VALUE* argv) const noexcept override {
    using traits = resolved_function_traits<Function>;
    using args = typename traits::args_tuple;
    if (parser_.configured() || has_special_args(std::make_index_sequence<traits::arity>{})) {
      return accepts_arity(argc) ? 0 : -1;
    }
    return tuple_match_score<args>(argc, argv, std::make_index_sequence<traits::arity>{});
  }

  [[nodiscard]] std::size_t declared_arity() const noexcept override {
    return resolved_function_traits<Function>::arity;
  }

private:
  template <std::size_t... Index>
  [[nodiscard]] value invoke_with_args(const parsed_arguments& parsed,
                                       std::index_sequence<Index...>) {
    using traits = resolved_function_traits<Function>;
    using args = typename traits::args_tuple;
    auto converted = std::tuple<decltype(load_parsed_argument<std::tuple_element_t<Index, args>>(
        parsed, Index))...>{
        load_parsed_argument<std::tuple_element_t<Index, args>>(parsed, Index)...};

    if constexpr (std::is_void_v<typename traits::return_type>) {
      std::apply(function_, converted);
      return value{Qnil};
    } else {
      decltype(auto) result = std::apply(function_, converted);
      return dump_result(std::forward<decltype(result)>(result), policy::kind::automatic);
    }
  }

  template <std::size_t... Index>
  static consteval bool has_special_args(std::index_sequence<Index...>) {
    using args = typename resolved_function_traits<Function>::args_tuple;
    return (is_special_argument_v<std::tuple_element_t<Index, args>> || ...);
  }

  template <std::size_t... Index>
  static consteval bool has_rest_args(std::index_sequence<Index...>) {
    using args = typename resolved_function_traits<Function>::args_tuple;
    return (is_rest_argument_v<std::tuple_element_t<Index, args>> || ...);
  }

  Function function_;
  argument_parser<typename resolved_function_traits<Function>::args_tuple> parser_;
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
    native_function* selected = nullptr;
    int best_score = -1;
    for (const auto& candidate : *functions) {
      int score = candidate->match_score(argc, argv);
      if (score > best_score) {
        best_score = score;
        selected = candidate.get();
      }
    }
    if (selected == nullptr && functions->size() == 1U && functions->front()->accepts_arity(argc)) {
      selected = functions->front().get();
    }
    if (selected == nullptr) {
      std::string message = "rbxx: no matching overload";
      if (functions->size() == 1U) {
        message += "; expected ";
        message += std::to_string(functions->front()->declared_arity());
        message += ", actual ";
        message += std::to_string(argc);
      }
      message += "; candidates:";
      for (const auto& candidate : *functions) {
        message += "\n  ";
        message += candidate->signature();
      }
      throw ruby_error(make_exception(rb_eArgError, message.c_str()));
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
void register_function(VALUE owner, const char* name, Function&& function,
                       std::vector<argument_spec> specs = {}, bool global = false) {
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
    const bool first =
        method_registry::instance().add(global ? Qnil : owner, method,
                                        std::make_unique<native_function_impl<stored_function>>(
                                            std::forward<Function>(function), std::move(specs)));
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
void register_static_function(VALUE owner, const char* name, Function&& function,
                              std::vector<argument_spec> specs = {}) {
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
    const bool first =
        method_registry::instance().add(owner, method,
                                        std::make_unique<native_function_impl<stored_function>>(
                                            std::forward<Function>(function), std::move(specs)));
    if (first) {
      protect([owner, name] { rb_define_singleton_method(owner, name, function_trampoline, -1); });
    }
  }
}

} // namespace rbxx::detail
