# frozen_string_literal: true

require "minitest/autorun"

GC.stress = true if ENV["RBXX_GC_STRESS"] == "1"

module RbxxTestHelpers
  def with_gc_stress
    previous = GC.stress
    GC.stress = true
    yield
  ensure
    GC.stress = previous
  end

  def compacted
    skip "GC compaction is unavailable" unless GC.respond_to?(:verify_compaction_references)

    result = yield
    GC.verify_compaction_references(expand_heap: true, toward: :empty)
    result
  end

  def assert_destructor_runs(counter_class)
    before = counter_class.destroyed
    yield
    5.times { GC.start }
    assert_operator counter_class.destroyed, :>, before
  end
end

Minitest::Test.include(RbxxTestHelpers)
