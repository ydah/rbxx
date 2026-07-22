#pragma once

#include <rbxx/object.hpp>

#include <exception>
#include <new>
#include <stdexcept>
#include <string>

namespace rbxx {

/// @brief A C++ exception that preserves the original Ruby exception object.
/// @code catch (const rbxx::ruby_error& error) { error.reraise(); } @endcode
class ruby_error : public std::exception {
public:
  /// @brief Pins a Ruby exception for propagation through C++ frames.
  explicit ruby_error(value exception) : exception_(exception) {}
  explicit ruby_error(VALUE exception) : exception_(exception) {}

  /// @brief Returns a stable description for C++ diagnostics.
  [[nodiscard]] const char* what() const noexcept override { return "Ruby exception"; }

  /// @brief Returns the preserved Ruby exception.
  [[nodiscard]] value exception() const noexcept { return exception_.get(); }

  /// @brief Returns the Ruby exception class name.
  [[nodiscard]] std::string ruby_class_name() const;

  /// @brief Returns the Ruby exception message.
  [[nodiscard]] std::string message() const;

  /// @brief Throws a copy so the outer Ruby boundary can transparently re-raise it.
  [[noreturn]] void reraise() const { throw *this; }

private:
  object exception_;
};

namespace detail {

inline VALUE exception_class_name(VALUE exception) { return rb_class_name(CLASS_OF(exception)); }

inline VALUE exception_message(VALUE exception) {
  VALUE message = rb_funcall(exception, rb_intern("message"), 0);
  return rb_obj_as_string(message);
}

inline std::string protected_exception_string(VALUE exception, VALUE (*function)(VALUE)) {
  int state = 0;
  VALUE string = rb_protect(function, exception, &state);
  if (state != 0) {
    VALUE nested = rb_errinfo();
    rb_set_errinfo(Qnil);
    throw ruby_error(nested);
  }
  return std::string(RSTRING_PTR(string), static_cast<std::size_t>(RSTRING_LEN(string)));
}

struct exception_creation {
  VALUE klass;
  const char* message;
};

inline VALUE create_exception(VALUE opaque) {
  auto* creation = reinterpret_cast<exception_creation*>(opaque);
  return rb_exc_new_cstr(creation->klass, creation->message);
}

inline VALUE make_exception(VALUE klass, const char* message) noexcept {
  exception_creation creation{klass, message};
  int state = 0;
  VALUE result = rb_protect(create_exception, reinterpret_cast<VALUE>(&creation), &state);
  if (state == 0) {
    return result;
  }

  result = rb_errinfo();
  rb_set_errinfo(Qnil);
  return result;
}

/// @brief Converts the active C++ exception into a Ruby exception VALUE without raising it.
/// @code catch (...) { pending = rbxx::detail::translate_current_exception(); } @endcode
inline VALUE translate_current_exception() noexcept {
  try {
    throw;
  } catch (const ruby_error& error) {
    return error.exception().raw();
  } catch (const std::invalid_argument& error) {
    return make_exception(rb_eArgError, error.what());
  } catch (const std::out_of_range& error) {
    return make_exception(rb_eRangeError, error.what());
  } catch (const std::range_error& error) {
    return make_exception(rb_eRangeError, error.what());
  } catch (const std::bad_alloc& error) {
    return make_exception(rb_eNoMemError, error.what());
  } catch (const std::domain_error& error) {
    return make_exception(rb_eMathDomainError, error.what());
  } catch (const std::exception& error) {
    return make_exception(rb_eRuntimeError, error.what());
  } catch (...) {
    return make_exception(rb_eRuntimeError, "unknown C++ exception");
  }
}

} // namespace detail

inline std::string ruby_error::ruby_class_name() const {
  return detail::protected_exception_string(exception_.raw(), detail::exception_class_name);
}

inline std::string ruby_error::message() const {
  return detail::protected_exception_string(exception_.raw(), detail::exception_message);
}

} // namespace rbxx
