# frozen_string_literal: true

require_relative "helper"
require "smoke/smoke"

class SmokeTest < Minitest::Test
  def test_native_extension_loads
    assert_equal true, RbxxTest::SMOKE
    assert_equal "0.1.0", RbxxTest::VERSION
  end
end
