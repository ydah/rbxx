# frozen_string_literal: true

require_relative "helper"
require "boundary/boundary"

class BoundaryTest < Minitest::Test
  Boundary = RbxxTest::Boundary

  def test_cpp_exception_becomes_ruby_exception
    error = assert_raises(RuntimeError) { Boundary.cpp_runtime_error }
    assert_equal "native failure", error.message
  end

  def test_protect_does_not_leak_cpp_exceptions_through_rb_protect
    error = assert_raises(RuntimeError) { Boundary.cpp_exception_through_protect }
    assert_equal "protected native failure", error.message
  end

  def test_default_cpp_exception_mapping
    assert_raises(ArgumentError) { Boundary.mapped_invalid_argument }
    assert_raises(RangeError) { Boundary.mapped_out_of_range }
    assert_raises(RangeError) { Boundary.mapped_range_error }
    assert_raises(Math::DomainError) { Boundary.mapped_domain_error }
    assert_raises(NoMemoryError) { Boundary.mapped_bad_alloc }
    error = assert_raises(RuntimeError) { Boundary.mapped_unknown }
    assert_equal "unknown C++ exception", error.message
  end

  def test_ruby_raise_unwinds_cpp_locals
    before = Boundary.destructor_count
    error = assert_raises(ArgumentError) do
      Boundary.call_with_guard(proc { raise ArgumentError, "ruby failure" })
    end

    assert_equal "ruby failure", error.message
    assert_equal before + 1, Boundary.destructor_count
  end

  def test_reraised_error_preserves_identity_and_backtrace
    original = ArgumentError.new("round trip")
    original.set_backtrace(["original.rb:12"])
    reraised = assert_raises(ArgumentError) do
      Boundary.call_and_reraise(proc { raise original })
    end

    assert_same original, reraised
    assert_equal ["original.rb:12"], reraised.backtrace
  end

  def test_ruby_error_exposes_class_and_message
    details = Boundary.inspect_ruby_error(proc { raise KeyError, "missing" })
    assert_equal %w[KeyError missing], details
  end

  def test_object_copy_and_move_pin_values
    with_gc_stress do
      assert_equal "copy survived", Boundary.object_copy_survives_gc
      assert_equal "move survived", Boundary.object_move_survives_gc
    end
  end

  def test_object_pin_is_updated_by_compaction
    skip "GC.compact is unavailable" unless GC.respond_to?(:compact)

    assert_equal "compaction survived", Boundary.object_survives_compaction
  end
end
