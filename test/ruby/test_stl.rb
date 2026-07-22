# frozen_string_literal: true

require_relative "helper"
require "stl/stl"

class StlTest < Minitest::Test
  Stl = RbxxTest::Stl

  def test_sequence_conversions
    assert_equal [1, 2, 3], Stl.vector([1, 2, 3])
    assert_equal [1, 2, 3], Stl.array([1, 2, 3])
    assert_raises(ArgumentError) { Stl.array([1, 2]) }
    assert_equal [1, 2, 3], Stl.set([3, 1, 2, 1])
  end

  def test_map_conversions
    expected = { "one" => 1, "two" => 2 }
    assert_equal expected, Stl.map(expected)
    assert_equal expected, Stl.unordered_map(expected)
  end

  def test_product_conversions
    assert_equal [1, "one"], Stl.pair([1, "one"])
    assert_equal [1, "one", true], Stl.tuple([1, "one", true])
    assert_raises(ArgumentError) { Stl.tuple([1]) }
  end

  def test_optional_and_variant
    assert_nil Stl.optional(nil)
    assert_equal 3, Stl.optional(3)
    assert_equal 4, Stl.variant(4)
    assert_equal "four", Stl.variant("four")
    assert_raises(TypeError) { Stl.variant(nil) }
  end

  def test_chrono_and_filesystem
    assert_in_delta 1.5, Stl.duration(1.5), 0.001
    assert_equal "tmp/example.txt", Stl.path("tmp/example.txt")
  end

  def test_recursive_nested_conversion
    nested = [{ "first" => 1, "missing" => nil }, { "second" => 2 }]
    assert_equal nested, Stl.nested(nested)
  end

  def test_ruby_conversion_protocols
    array_like = Object.new
    array_like.define_singleton_method(:to_ary) { [1, 2, 3] }
    hash_like = Object.new
    hash_like.define_singleton_method(:to_hash) { { "one" => 1 } }
    string_like = Object.new
    string_like.define_singleton_method(:to_str) { "converted" }

    assert_equal [1, 2, 3], Stl.vector(array_like)
    assert_equal({ "one" => 1 }, Stl.map(hash_like))
    assert_equal "converted", Stl.string(string_like)
  end

  def test_large_vector_conversion
    skip "large conversion is covered by the regular run" if ENV["RBXX_GC_STRESS"] == "1"

    input = Array.new(1_000_000) { |index| index }
    assert_equal input, Stl.vector(input)
  end
end
