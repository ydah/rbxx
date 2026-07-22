# frozen_string_literal: true

require_relative "helper"
require "arguments/arguments"

class ArgumentsTest < Minitest::Test
  Arguments = RbxxTest::Arguments

  def test_positional_default
    assert_equal 13, Arguments.add_default(3)
    assert_equal 7, Arguments.add_default(3, 4)
    assert_raises(ArgumentError) { Arguments.add_default }
  end

  def test_mixed_positional_and_keyword_arguments
    assert_equal "xxx", Arguments.mixed("x", count: 3)
    assert_equal "xx!", Arguments.mixed("x", count: 2, bang: true)
    assert_raises(ArgumentError) { Arguments.mixed("x") }
    assert_raises(ArgumentError) { Arguments.mixed("x", count: 1, unknown: true) }
  end

  def test_required_and_optional_blocks
    assert_equal 8, Arguments.apply_block(2) { |value| value * 2 }
    assert_raises(ArgumentError) { Arguments.apply_block(2) }
    assert_equal 3, Arguments.apply_optional_block(3)
    assert_equal 9, Arguments.apply_optional_block(3) { |value| value * 3 }
  end

  def test_splat_arguments
    assert_equal 10, Arguments.sum_rest(1, 2, 3, 4)
    assert_equal 1, Arguments.sum_rest(1)
  end

  def test_overload_resolution
    assert_equal "integer", Arguments.choose(1)
    assert_equal "string", Arguments.choose("one")
    assert_equal "token", Arguments.choose(Arguments::Token.new("one"))
    assert_equal "first", Arguments.definition_order(1)

    error = assert_raises(ArgumentError) { Arguments.choose(nil) }
    assert_match(/no matching overload/, error.message)
    assert_match(/candidates:/, error.message)
  end

  def test_keyword_constructor_experience
    assert_equal 0, Arguments::NumberBox.new.value
    assert_equal 10, Arguments::NumberBox.new(start: 10).value
  end

  def test_keyword_member_default
    box = Arguments::NumberBox.new(start: 4)
    assert_equal 8, box.scale
    assert_equal 12, box.scale(factor: 3)
    assert_raises(ArgumentError) { box.scale(extra: 2) }
  end

  def test_operator_bindings_and_comparable
    box = Arguments::NumberBox.new(start: 4)
    sum = box + 3
    assert_equal 7, sum.value
    assert_equal(-1, box <=> sum)
    assert_operator box, :<, sum
    assert_equal 6, box[2]
  end

  def test_compile_time_fast_path_falls_back_when_overloaded
    object = Arguments::OverloadedMember.new

    assert_equal 1, object.pick
    assert_equal 5, object.pick(4)
  end
end
