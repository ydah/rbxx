# frozen_string_literal: true

require_relative "helper"
require "conversions/conversions"

class ConversionsTest < Minitest::Test
  Conversions = RbxxTest::Conversions

  def test_boolean_round_trip_is_strict
    assert_equal true, Conversions.bool(true)
    assert_equal false, Conversions.bool(false)
    assert_raises(TypeError) { Conversions.bool(nil) }
    assert_raises(TypeError) { Conversions.bool(1) }
  end

  def test_small_signed_integer_boundaries
    assert_equal(-128, Conversions.int8(-128))
    assert_equal 127, Conversions.int8(127)
    assert_raises(RangeError) { Conversions.int8(-129) }
    assert_raises(RangeError) { Conversions.int8(128) }
  end

  def test_large_signed_integer_boundaries
    assert_equal((2**63) - 1, Conversions.int64((2**63) - 1))
    assert_equal(-(2**63), Conversions.int64(-(2**63)))
    assert_raises(RangeError) { Conversions.int32(2**63) }
  end

  def test_unsigned_integer_boundaries
    assert_equal 0, Conversions.uint8(0)
    assert_equal 255, Conversions.uint8(255)
    assert_raises(RangeError) { Conversions.uint8(-1) }
    assert_raises(RangeError) { Conversions.uint8(256) }
    assert_equal((2**64) - 1, Conversions.uint64((2**64) - 1))
    assert_raises(RangeError) { Conversions.uint32(2**64) }
  end

  def test_numeric_nil_is_rejected
    assert_raises(TypeError) { Conversions.int32(nil) }
    assert_raises(TypeError) { Conversions.double(nil) }
  end

  def test_float_round_trip
    assert_in_delta 1.25, Conversions.float(1.25), 0.000_01
    assert_in_delta Math::PI, Conversions.double(Math::PI), Float::EPSILON
  end

  def test_utf8_string_round_trip
    value = "こんにちは"
    assert_equal value, Conversions.cstring(value)
    assert_equal value, Conversions.string(value)
    assert_equal value, Conversions.string_view(value)
    assert_equal Encoding::UTF_8, Conversions.string(value).encoding
    assert_raises(TypeError) { Conversions.string(nil) }
    assert_raises(ArgumentError) { Conversions.string("a\0b") }
  end

  def test_value_conversion_is_identity
    object = Object.new
    assert_same object, Conversions.value(object)
  end

  def test_matches_identifies_candidates
    assert_equal 3, Conversions.match_mask(1)
    assert_equal 2, Conversions.match_mask(1.0)
    assert_equal 4, Conversions.match_mask("one")
    assert_equal 8, Conversions.match_mask(true)
  end
end
