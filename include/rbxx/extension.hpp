#pragma once

#include <rbxx/exception.hpp>

#if defined(_WIN32)
#define RBXX_EXPORT __declspec(dllexport)
#else
#define RBXX_EXPORT __attribute__((visibility("default")))
#endif

#define RBXX_EXTENSION(name)                                                                       \
  static void rbxx_init_body_##name();                                                             \
  extern "C" RBXX_EXPORT void Init_##name() {                                                      \
    VALUE rbxx_pending_exception = Qnil;                                                           \
    try {                                                                                          \
      rbxx_init_body_##name();                                                                     \
      return;                                                                                      \
    } catch (...) {                                                                                \
      rbxx_pending_exception = ::rbxx::detail::translate_current_exception();                      \
    }                                                                                              \
    rb_exc_raise(rbxx_pending_exception);                                                          \
  }                                                                                                \
  static void rbxx_init_body_##name()
