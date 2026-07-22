#pragma once

#include <rbxx/policies.hpp>
#include <rbxx/registry.hpp>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace rbxx {

/// @brief A Ruby VALUE member that remains marked and compaction-safe inside a C++ object.
/// @code rbxx::member_value child{ruby_child}; @endcode
class member_value {
public:
  member_value() noexcept { rb_gc_register_address(&stored_); }
  explicit member_value(value initial) noexcept : stored_(initial.raw()) {
    rb_gc_register_address(&stored_);
  }
  explicit member_value(VALUE initial) noexcept : stored_(initial) {
    rb_gc_register_address(&stored_);
  }
  member_value(const member_value& other) noexcept : stored_(other.stored_) {
    rb_gc_register_address(&stored_);
  }
  member_value(member_value&& other) noexcept : stored_(other.stored_) {
    rb_gc_register_address(&stored_);
    other.stored_ = Qnil;
  }
  member_value& operator=(const member_value& other) noexcept {
    stored_ = other.stored_;
    return *this;
  }
  member_value& operator=(member_value&& other) noexcept {
    stored_ = other.stored_;
    other.stored_ = Qnil;
    return *this;
  }
  ~member_value() noexcept { rb_gc_unregister_address(&stored_); }

  [[nodiscard]] value get() const noexcept { return value{stored_}; }
  void set(value updated) noexcept { stored_ = updated.raw(); }

private:
  VALUE stored_ = Qnil;
};

namespace detail {

template <typename T> struct is_ownership_pointer : std::false_type {};
template <typename T> struct is_ownership_pointer<std::unique_ptr<T>> : std::true_type {};
template <typename T> struct is_ownership_pointer<std::shared_ptr<T>> : std::true_type {};

enum class ownership { owned, borrowed, shared };

template <typename T> struct data_wrapper {
  T* pointer = nullptr;
  ownership mode = ownership::borrowed;
  std::shared_ptr<T> shared_owner;

  ~data_wrapper() noexcept {
    if (mode == ownership::owned) {
      delete pointer;
    }
  }
};

template <typename T> void data_mark(void*) noexcept {}
template <typename T> void data_compact(void*) noexcept {}

template <typename T> void data_free(void* raw) noexcept {
  delete static_cast<data_wrapper<T>*>(raw);
}

template <typename T> std::size_t data_size(const void* raw) noexcept {
  const auto* wrapper = static_cast<const data_wrapper<T>*>(raw);
  return sizeof(data_wrapper<T>) +
         (wrapper != nullptr && wrapper->pointer != nullptr ? sizeof(T) : 0U);
}

template <typename T> rb_data_type_t& typed_data_type() {
  static auto* name = new std::string("rbxx::" + std::string(type_name<T>()));
  static rb_data_type_t type = {
      name->c_str(),
      {data_mark<T>, data_free<T>, data_size<T>, data_compact<T>, {nullptr}},
      nullptr,
      nullptr,
      RUBY_TYPED_FREE_IMMEDIATELY,
  };
  return type;
}

template <typename T> value wrap_data(std::unique_ptr<data_wrapper<T>> wrapper) {
  class_info& information = registered_class<T>();
  VALUE result = protect(rb_data_typed_object_wrap, information.ruby_class,
                         static_cast<void*>(wrapper.get()), information.data_type);
  wrapper.release();
  return value{result};
}

template <typename T> value wrap_copy(const T& input) {
  auto wrapper = std::make_unique<data_wrapper<T>>();
  wrapper->pointer = new T(input);
  wrapper->mode = ownership::owned;
  return wrap_data(std::move(wrapper));
}

template <typename T> value wrap_move(T&& input) {
  auto wrapper = std::make_unique<data_wrapper<T>>();
  wrapper->pointer = new T(std::move(input));
  wrapper->mode = ownership::owned;
  return wrap_data(std::move(wrapper));
}

template <typename T> value wrap_reference(T* input) {
  if (input == nullptr) {
    return value{Qnil};
  }
  auto wrapper = std::make_unique<data_wrapper<T>>();
  wrapper->pointer = input;
  wrapper->mode = ownership::borrowed;
  return wrap_data(std::move(wrapper));
}

template <typename T> value wrap_take(std::unique_ptr<T> input) {
  if (!input) {
    return value{Qnil};
  }
  auto wrapper = std::make_unique<data_wrapper<T>>();
  wrapper->pointer = input.release();
  wrapper->mode = ownership::owned;
  return wrap_data(std::move(wrapper));
}

template <typename T> value wrap_shared(std::shared_ptr<T> input) {
  if (!input) {
    return value{Qnil};
  }
  auto wrapper = std::make_unique<data_wrapper<T>>();
  wrapper->pointer = input.get();
  wrapper->mode = ownership::shared;
  wrapper->shared_owner = std::move(input);
  return wrap_data(std::move(wrapper));
}

template <typename T, typename Base = void>
void register_data_class(VALUE ruby_class,
                         std::source_location location = std::source_location::current()) {
  rb_data_type_t& type = typed_data_type<T>();
  if constexpr (!std::is_void_v<Base>) {
    type.parent = &typed_data_type<Base>();
  }
  type_registry::instance().add<T, Base>(ruby_class, &type, location);
  type_registry::instance().set_native_pointer<T>(
      [](void* raw) { return static_cast<void*>(static_cast<data_wrapper<T>*>(raw)->pointer); });
}

template <typename T> VALUE allocate_data_object(VALUE klass) {
  VALUE result = Qnil;
  VALUE pending = Qnil;
  try {
    auto wrapper = std::make_unique<data_wrapper<T>>();
    result = protect(rb_data_typed_object_wrap, klass, static_cast<void*>(wrapper.get()),
                     &typed_data_type<T>());
    wrapper.release();
  } catch (...) {
    pending = translate_current_exception();
  }
  if (!NIL_P(pending)) {
    rb_exc_raise(pending);
  }
  return result;
}

template <typename T> data_wrapper<T>& exact_wrapper(value self) {
  if (!RTYPEDDATA_P(self.raw()) || RTYPEDDATA_TYPE(self.raw()) != &typed_data_type<T>()) {
    throw ruby_error(make_exception(rb_eTypeError, "rbxx: unexpected TypedData class"));
  }
  return *static_cast<data_wrapper<T>*>(RTYPEDDATA_DATA(self.raw()));
}

} // namespace detail

template <typename T>
struct type_caster<T, std::enable_if_t<std::is_class_v<T> && !std::is_same_v<T, object> &&
                                       !detail::is_ownership_pointer<T>::value>> {
  static constexpr std::string_view name = "bound C++ object";
  static T& load(value input) { return detail::load_registered<T>(input); }
  static value dump(const T& input) { return detail::wrap_copy(input); }
  static value dump(T&& input) { return detail::wrap_move(std::move(input)); }
  static bool matches(value input) noexcept { return RTYPEDDATA_P(input.raw()); }
};

template <typename T> struct type_caster<T*> {
  static constexpr std::string_view name = "bound C++ object or nil";
  static T* load(value input) {
    return input.is_nil() ? nullptr : std::addressof(detail::load_registered<T>(input));
  }
  static value dump(T* input) { return detail::wrap_reference(input); }
  static bool matches(value input) noexcept { return input.is_nil() || RTYPEDDATA_P(input.raw()); }
};

template <typename T> struct type_caster<std::unique_ptr<T>> {
  static constexpr std::string_view name = "owned C++ object";
  static value dump(std::unique_ptr<T> input) { return detail::wrap_take(std::move(input)); }
  static bool matches(value) noexcept { return false; }
};

template <typename T> struct type_caster<std::shared_ptr<T>> {
  static constexpr std::string_view name = "shared C++ object";
  static value dump(std::shared_ptr<T> input) { return detail::wrap_shared(std::move(input)); }
  static bool matches(value input) noexcept { return RTYPEDDATA_P(input.raw()); }
};

} // namespace rbxx
