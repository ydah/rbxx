# Benchmarks

## STL conversion baseline

Measured on 2026-07-22 with Ruby 4.0.0, Apple arm64, and Apple Clang/C++20. Run with
`RUBYLIB=tmp/test_extensions bundle exec ruby bench/stl.rb`.

| Conversion | Throughput | Time per iteration |
|---|---:|---:|
| `vector<int>` 1,000 elements | 10,488 i/s | 95.34 µs |
| `vector<int>` 1,000,000 elements | 10.54 i/s | 94.84 ms |

The million-element case verifies that conversion is linear and uses one outer Ruby protection
boundary plus immediate-integer fast paths, rather than one `rb_protect` call per element.

## GVL release

Measured on the same system with two Ruby threads, each running 150 ms of native blocking work.
The `rbxx::nogvl` adapter completed both calls in 0.155 s. A serialized execution would take at
least 0.300 s, so the result confirms that the native work overlaps while argument and return-value
conversion remain under the GVL.
