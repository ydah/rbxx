# frozen_string_literal: true

require_relative "helper"
require "advanced/advanced"

class AdvancedTest < Minitest::Test
  Advanced = RbxxTest::Advanced

  def test_keep_alive_preserves_borrowed_result_owner
    before = Advanced::Owner.destroyed
    borrowed = Advanced::Owner.new(11).child

    5.times { GC.start }

    assert_equal 11, borrowed.value
    assert_equal before, Advanced::Owner.destroyed
  end

  def test_unsafe_reference_reproduces_uaf_only_when_explicitly_enabled
    skip "set RBXX_DANGER_TESTS=1 under ASAN to run the intentional UAF" unless ENV["RBXX_DANGER_TESTS"] == "1"

    borrowed = Advanced::Owner.new(12).unsafe_child
    5.times { GC.start }
    assert_equal 12, borrowed.value
  end

  def test_proc_callback_and_return_conversion
    assert_equal 14, Advanced.apply_callback(->(value) { value * 2 }, 7)
  end

  def test_callback_preserves_original_ruby_exception
    original = Class.new(StandardError)
    error = assert_raises(original) do
      Advanced.apply_callback(->(_value) { raise original, "from callback" }, 1)
    end
    assert_equal "from callback", error.message
  end

  def test_debug_callback_rejects_invocation_without_gvl
    error = assert_raises(RuntimeError) do
      Advanced.apply_callback_without_gvl(->(value) { value })
    end
    assert_match(/without the GVL/, error.message)
  end

  def test_nogvl_allows_ruby_threads_to_run_concurrently
    skip "wall-clock concurrency assertion is unstable under GC.stress" if ENV["RBXX_GC_STRESS"] == "1"

    started = Process.clock_gettime(Process::CLOCK_MONOTONIC)
    threads = 2.times.map { Thread.new { Advanced.blocking_work(250) } }
    assert_equal [250, 250], threads.map(&:value)
    elapsed = Process.clock_gettime(Process::CLOCK_MONOTONIC) - started

    assert_operator elapsed, :<, 0.42, "expected parallel work, took #{elapsed.round(3)}s"
  end

  def test_nogvl_interrupt_hook_stops_native_work
    thread = Thread.new { Advanced.interruptible_work(5_000) }
    thread.report_on_exception = false
    sleep 0.05
    started = Process.clock_gettime(Process::CLOCK_MONOTONIC)
    thread.raise(Interrupt, "stop")

    assert_raises(Interrupt) { thread.value }
    elapsed = Process.clock_gettime(Process::CLOCK_MONOTONIC) - started
    assert_operator elapsed, :<, 0.5, "interrupt hook took #{elapsed.round(3)}s"
  end

  def test_iterable_includes_enumerable_and_reports_size
    buffer = Advanced::IntBuffer.new([1, 2, 3, 4])

    assert_kind_of Enumerable, buffer
    assert_equal [1, 2, 3, 4], buffer.to_a
    doubled = buffer.map { |value| value * 2 }
    assert_equal [2, 4, 6, 8], doubled
    assert_equal 4, buffer.each.size
  end
end
