#pragma once

#include <rbxx/type_caster.hpp>

#include <functional>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace rbxx::detail {

struct class_info {
  std::type_index cpp_type{typeid(void)};
  VALUE ruby_class = Qnil;
  const rb_data_type_t* data_type = nullptr;
  std::function<void*(void*)> native_pointer;
  std::unordered_map<std::type_index, std::function<void*(void*)>> conversions;
  std::source_location registered_at = std::source_location::current();
};

class type_registry {
public:
  static type_registry& instance() {
    static auto* registry = new type_registry();
    return *registry;
  }

  template <typename T, typename Base = void>
  void add(VALUE ruby_class, const rb_data_type_t* data_type,
           std::source_location location = std::source_location::current()) {
    auto information = std::make_unique<class_info>();
    information->cpp_type = std::type_index(typeid(T));
    information->ruby_class = ruby_class;
    information->data_type = data_type;
    information->conversions.emplace(std::type_index(typeid(T)),
                                     [](void* pointer) { return pointer; });
    information->registered_at = location;

    if constexpr (!std::is_void_v<Base>) {
      class_info* base = find(std::type_index(typeid(Base)));
      if (base == nullptr) {
        std::string message = "rbxx: base C++ type ";
        message += type_name<Base>();
        message += " is not registered; call def_class<Base>() first";
        throw ruby_error(make_exception(rb_eTypeError, message.c_str()));
      }
      for (const auto& [target, conversion] : base->conversions) {
        information->conversions.emplace(target, [conversion](void* pointer) {
          auto* derived = static_cast<T*>(pointer);
          return conversion(static_cast<Base*>(derived));
        });
      }
    }

    std::lock_guard lock(write_mutex_);
    class_info* stored = information.get();
    by_data_type_.insert_or_assign(data_type, stored);
    by_type_.insert_or_assign(std::type_index(typeid(T)), std::move(information));
  }

  template <typename T> void set_native_pointer(std::function<void*(void*)> extractor) {
    class_info* information = find(std::type_index(typeid(T)));
    if (information != nullptr) {
      information->native_pointer = std::move(extractor);
    }
  }

  [[nodiscard]] class_info* find(std::type_index type) const noexcept {
    auto found = by_type_.find(type);
    return found == by_type_.end() ? nullptr : found->second.get();
  }

  [[nodiscard]] class_info* find(const rb_data_type_t* type) const noexcept {
    auto found = by_data_type_.find(type);
    return found == by_data_type_.end() ? nullptr : found->second;
  }

private:
  std::mutex write_mutex_;
  std::unordered_map<std::type_index, std::unique_ptr<class_info>> by_type_;
  std::unordered_map<const rb_data_type_t*, class_info*> by_data_type_;
};

template <typename T> [[noreturn]] void throw_unregistered_type() {
  std::string message = "rbxx: C++ type ";
  message += type_name<T>();
  message += " is not registered; call def_class<T>() or specialize rbxx::type_caster<T>";
  throw ruby_error(make_exception(rb_eTypeError, message.c_str()));
}

template <typename T> class_info& registered_class() {
  class_info* information = type_registry::instance().find(std::type_index(typeid(T)));
  if (information == nullptr) {
    throw_unregistered_type<T>();
  }
  return *information;
}

template <typename T> T& load_registered(value input) {
  VALUE raw = input.raw();
  if (!RB_TYPE_P(raw, T_DATA) || !RTYPEDDATA_P(raw)) {
    std::string message = "rbxx: expected registered C++ type ";
    message += type_name<T>();
    message += ", got ";
    message += rb_obj_classname(raw);
    message += "; call def_class<T>() for the expected type";
    throw ruby_error(make_exception(rb_eTypeError, message.c_str()));
  }

  class_info* actual = type_registry::instance().find(RTYPEDDATA_TYPE(raw));
  if (actual == nullptr || !actual->native_pointer) {
    throw_unregistered_type<T>();
  }
  auto conversion = actual->conversions.find(std::type_index(typeid(T)));
  if (conversion == actual->conversions.end()) {
    std::string message = "rbxx: wrapped C++ type cannot be converted to ";
    message += type_name<T>();
    message += "; register the inheritance with def_class<Derived, Base>()";
    message += "; actual type was registered at ";
    message += actual->registered_at.file_name();
    message += ":";
    message += std::to_string(actual->registered_at.line());
    throw ruby_error(make_exception(rb_eTypeError, message.c_str()));
  }

  void* native = actual->native_pointer(RTYPEDDATA_DATA(raw));
  if (native == nullptr) {
    throw ruby_error(make_exception(rb_eRuntimeError, "rbxx: C++ object is not initialized"));
  }
  return *static_cast<T*>(conversion->second(native));
}

template <typename T> bool registered_matches(value input) noexcept {
  if (!RB_TYPE_P(input.raw(), T_DATA) || !RTYPEDDATA_P(input.raw())) {
    return false;
  }
  class_info* actual = type_registry::instance().find(RTYPEDDATA_TYPE(input.raw()));
  return actual != nullptr &&
         actual->conversions.contains(std::type_index(typeid(std::remove_cv_t<T>)));
}

} // namespace rbxx::detail
