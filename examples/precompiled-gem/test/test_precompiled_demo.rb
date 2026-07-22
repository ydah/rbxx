# frozen_string_literal: true

require "minitest/autorun"
require "precompiled_demo"

class PrecompiledDemoTest < Minitest::Test
  def test_native_function
    assert_equal 42, PrecompiledDemo.multiply(6, 7)
  end
end
