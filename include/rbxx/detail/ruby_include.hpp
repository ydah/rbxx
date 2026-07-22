#pragma once

#if defined(_MSC_VER)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#pragma warning(push)
#pragma warning(disable : 4127 4244 4267 4996)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <ruby.h>
#include <ruby/thread.h>
#include <ruby/version.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

#define RBXX_RUBY_VERSION_MAJOR RUBY_API_VERSION_MAJOR
#define RBXX_RUBY_VERSION_MINOR RUBY_API_VERSION_MINOR
#define RBXX_RUBY_VERSION_TEENY RUBY_API_VERSION_TEENY

#if RUBY_API_VERSION_MAJOR < 4
// CRuby exports this debug helper before 4.0 but does not declare it in ruby/thread.h.
extern "C" int ruby_thread_has_gvl_p(void);
#endif

static_assert(RUBY_API_VERSION_MAJOR >= 3, "rbxx requires CRuby 3.1 or newer");
