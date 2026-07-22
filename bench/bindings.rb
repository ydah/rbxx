# frozen_string_literal: true

require "benchmark/ips"
require "bench_c"
require "bench_rbxx"
require "bench_rice"

WARMUP = Float(ENV.fetch("BENCH_WARMUP", "1"))
TIME = Float(ENV.fetch("BENCH_TIME", "2"))
TEXT = ("rbxx benchmark " * 8).freeze
VECTOR = Array.new(1_000) { |index| index.fdiv(10) }.freeze

def compare(title)
  puts "\n#{title}"
  Benchmark.ips do |benchmark|
    benchmark.config(warmup: WARMUP, time: TIME)
    yield benchmark
    benchmark.compare!
  end
end

counter_c = BenchC::Counter.new(42)
counter_rbxx = BenchRbxx::Counter.new(42)
counter_rice = BenchRice::Counter.new(42)

compare("zero-argument int method") do |benchmark|
  benchmark.report("hand C") { counter_c.value }
  benchmark.report("rbxx") { counter_rbxx.value }
  benchmark.report("Rice") { counter_rice.value }
end

compare("string roundtrip") do |benchmark|
  benchmark.report("hand C") { BenchC.string_roundtrip(TEXT) }
  benchmark.report("rbxx") { BenchRbxx.string_roundtrip(TEXT) }
  benchmark.report("Rice") { BenchRice.string_roundtrip(TEXT) }
end

compare("vector<double> 1,000 roundtrip") do |benchmark|
  benchmark.report("hand C") { BenchC.vector_roundtrip(VECTOR) }
  benchmark.report("rbxx") { BenchRbxx.vector_roundtrip(VECTOR) }
  benchmark.report("Rice") { BenchRice.vector_roundtrip(VECTOR) }
end

compare("object construction") do |benchmark|
  benchmark.report("hand C") { BenchC::Counter.new(1) }
  benchmark.report("rbxx") { BenchRbxx::Counter.new(1) }
  benchmark.report("Rice") { BenchRice::Counter.new(1) }
end
