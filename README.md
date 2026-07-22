# rbxx

[![CI](https://github.com/ydah/rbxx/actions/workflows/ci.yml/badge.svg)](https://github.com/ydah/rbxx/actions/workflows/ci.yml)

rbxx is a header-only C++20 binding library for CRuby native extensions. It combines explicit
Ruby/C++ exception boundaries, TypedData ownership, STL conversion, keyword arguments, callbacks,
GVL release, and source/precompiled gem tooling.

## Quick start

Install rbxx, then generate a native gem:

```sh
gem install rbxx
rbxx new counter_ext
cd counter_ext
bundle exec rake test
```

A complete class binding stays small:

```cpp
#include <rbxx/rbxx.hpp>
class Counter {
public:
  explicit Counter(int n) : n_(n) {}
  void add(int d) { n_ += d; }
  int value() const { return n_; }
private:
  int n_;
};
RBXX_EXTENSION(counter_ext) {
  rbxx::define_module("Demo").def_class<Counter>("Counter")
    .def(rbxx::init<int>(), rbxx::kwarg("start") = 0)
    .def("add", &Counter::add, rbxx::arg("delta"))
    .def<&Counter::value>("value");
}
```

Use `def<&T::method>` for the fixed-arity zero-argument fast path. Ordinary `.def(...)` supports
arguments, defaults, keywords, blocks, overloads, policies, and lifetime rules.

## Build integrations

- `require "rbxx/mkmf"` and `create_rbxx_makefile("gem/extension")`
- `find_package(rbxx CONFIG REQUIRED)` and the `rbxx::rbxx` CMake target
- `rbxx new NAME [--cmake]` for tested gem scaffolding
- `Rbxx::RakeTasks` for five rake-compiler-dock targets
- `single_include/rbxx/rbxx.hpp` when a generated single header is preferable

See [the tutorial](docs/tutorial.md), [API overview](docs/api.md), [examples](examples), and
[benchmark results](docs/benchmarks.md).

## Supported scope and limitations

- CRuby 3.1 and newer; C++20; GCC 12+, Clang 15+, AppleClang 15+, and MSVC 2022.
- Version 0.1 is main-Ractor only and does not call `rb_ext_ractor_safe`.
- Embedding Ruby in a C++ executable is not a supported v0.1 workflow.
- mruby, TruffleRuby, JRuby, C++ modules, and binding auto-generation are out of scope.
- Iterating a container while mutating it from Ruby has undefined behavior.
- A Ruby callback may only run on a Ruby thread holding the GVL.

## Development

Run `bundle exec rake compile:test test test:compile_fail`, `bundle exec rake test:gc_stress`,
`bundle exec rake lint`, and `bundle exec rake amalgamate:check amalgamate:smoke` before release.
Sanitizer CI uses `RBXX_SANITIZE=1`; intentional UAF reproduction is gated by
`RBXX_DANGER_TESTS=1` and is never part of the normal suite.

## License

rbxx is available under the MIT License.
