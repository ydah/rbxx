# Benchmarks

## Binding overhead

Measured on 2026-07-22 with Ruby 4.0.0, Apple arm64, AppleClang, `-O3`, rbxx 0.1.0,
Rice 4.12.0, benchmark-ips 2.15.1, one second of warmup, and two seconds of measurement.
Run with `bundle exec rake bench:run`.

| Operation | Hand C | rbxx | Rice | rbxx / C time | rbxx / Rice throughput |
|---|---:|---:|---:|---:|---:|
| zero-argument int method | 41.68M i/s | 41.17M i/s | 13.96M i/s | 1.01x | 2.95x |
| string roundtrip | 23.56M i/s | 7.22M i/s | 5.39M i/s | 3.27x | 1.34x |
| `vector<double>` 1,000 roundtrip | 155.72k i/s | 187.82k i/s | 51.18k i/s | 0.83x | 3.67x |
| object construction | 16.43M i/s | 6.53M i/s | 3.44M i/s | 2.51x | 1.90x |

The G2 latency target applies to the zero-argument method hot path. The compile-time
`def<&T::method>` trampoline is statistically level with hand-written C and remains well inside the
1.3x limit. General calls retain conversion and dispatch machinery; all four rbxx cases are faster
than Rice in this run. Float vectors use direct, non-raising access for values already known to be
Ruby Floats while one outer protection boundary covers allocation and Array insertion.

## Compile time

`bundle exec rake bench:compile_time` performs a syntax-only C++20 compile of the representative
kwargs/overload/operator binding in `test/cpp/arguments.cpp`. It completed in **0.714 s** on the
same machine. CI prints this value on every toolchain run so growth is visible without imposing a
machine-dependent hard threshold.

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
