# frozen_string_literal: true

require "benchmark"
require "advanced/advanced"

module NogvlBenchmark
  module_function

  def measure(label)
    started = Process.clock_gettime(Process::CLOCK_MONOTONIC)
    yield
    elapsed = Process.clock_gettime(Process::CLOCK_MONOTONIC) - started
    puts format("%-24s %.3f s", label, elapsed)
  end
end

NogvlBenchmark.measure("two Ruby threads") do
  2.times.map { Thread.new { RbxxTest::Advanced.blocking_work(150) } }.each(&:join)
end
