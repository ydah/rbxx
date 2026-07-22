# frozen_string_literal: true

require "benchmark/ips"

begin
  require "stl/stl"
rescue LoadError
  warn "Run `bundle exec rake compile:test` and add tmp/test_extensions to RUBYLIB first"
  exit 1
end

small = Array.new(1_000) { |index| index }
large = Array.new(1_000_000) { |index| index }

Benchmark.ips do |benchmark|
  benchmark.config(time: 2, warmup: 1)
  benchmark.report("vector<int> 1k") { RbxxTest::Stl.vector(small) }
  benchmark.report("vector<int> 1m") { RbxxTest::Stl.vector(large) }
  benchmark.compare!
end
