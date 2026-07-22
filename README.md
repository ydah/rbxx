# rbxx

[![CI](https://github.com/ydah/rbxx/actions/workflows/ci.yml/badge.svg)](https://github.com/ydah/rbxx/actions/workflows/ci.yml)

rbxx is a header-only C++20 binding library for writing safe, fast CRuby native extensions. It provides explicit Ruby/C++ exception boundaries, GC-aware object handles, type casters, class and function binding, and tooling for source and precompiled gems.

The project is under active development toward v0.1.0. See the documentation in `docs/` for supported APIs and known limitations.

## Development

Run `bundle exec rake compile:test test`, `bundle exec rake test:gc_stress`, and `bundle exec rake lint` before committing.

## License

rbxx is available under the MIT License.
