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
    options = { toward: :empty }
    parameters = GC.method(:verify_compaction_references).parameters
    heap_option = parameters.include?(%i[key expand_heap]) ? :expand_heap : :double_heap
    GC.verify_compaction_references(**options, heap_option => true)
    result
  end

  def assert_destructor_runs(counter_class, &)
    before = counter_class.destroyed
    # Older Rubies can retain the most recent temporary in a VM stack slot.
    # Multiple instances ensure that at least one object is truly unreachable.
    32.times(&)
    5.times { GC.start }
    assert_operator counter_class.destroyed, :>, before
  end
end

Minitest::Test.include(RbxxTestHelpers)
