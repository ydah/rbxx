# frozen_string_literal: true

require_relative "helper"
require "functions/functions"

class FunctionsTest < Minitest::Test
  Functions = RbxxTest::Functions

  def test_zero_to_five_arguments
    assert_equal 42, Functions.zero
    assert_equal 1, Functions.one(1)
    assert_equal 3, Functions.two(1, 2)
    assert_equal 6, Functions.three(1, 2, 3)
    assert_equal 10, Functions.four(1, 2, 3, 4)
    assert_equal 15, Functions.five(1, 2, 3, 4, 5)
  end

  def test_reference_arguments
    assert_equal 2, Functions.mutable_reference(1)
    assert_equal "hello!", Functions.constant_reference("hello")
  end

  def test_void_return
    assert_nil Functions.set_last_value(7)
    assert_equal 7, Functions.read_last_value
  end

  def test_lambda_and_std_function
    assert_equal 8, Functions.lambda_value(4)
    assert_equal 14, Functions.std_function(4)
  end

  def test_cpp_exception_crosses_boundary
    error = assert_raises(RuntimeError) { Functions.throws_exception }
    assert_equal "function failure", error.message
  end

  def test_argument_count_error_is_actionable
    error = assert_raises(ArgumentError) { Functions.two(1) }
    assert_match(/expected 2, actual 1/, error.message)
  end

  def test_global_function
    assert_equal 9, rbxx_global_sum(4, 5)
  end
end
