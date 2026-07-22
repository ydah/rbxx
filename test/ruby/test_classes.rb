# frozen_string_literal: true

require_relative "helper"
require "classes/classes"

class ClassesTest < Minitest::Test
  Classes = RbxxTest::Classes

  def test_multiple_constructor_arities_and_methods
    assert_equal 0, Classes::Counter.new.value
    counter = Classes::Counter.new(10)
    counter.add(5)
    assert_equal 15, counter.value
  end

  def test_attribute_bindings
    counter = Classes::Counter.new(3)
    assert_equal 3, counter.number
    assert_equal 9, counter.number = 9
    assert_equal 9, counter.number
  end

  def test_destructor_runs_during_gc
    assert_destructor_runs(Classes::Counter) { Classes::Counter.new(1) }
  end

  def test_cpp_and_ruby_inheritance
    derived = Classes::Derived.new(6)
    assert_kind_of Classes::Base, derived
    assert_equal 6, derived.base_value
    assert_equal 12, derived.doubled
    assert_equal 6, Classes.read_base(derived)
  end

  def test_ruby_subclass_can_override_initialize_and_call_super
    subclass = Class.new(Classes::Counter) do
      attr_reader :ruby_initialized

      def initialize(value)
        @ruby_initialized = true
        super
      end
    end

    instance = subclass.new(7)
    assert_equal true, instance.ruby_initialized
    assert_equal 7, instance.value
  end

  def test_reference_policy_works_while_owner_is_alive
    owner = Classes::Owner.new(11)
    first = owner.child
    second = owner.child
    assert_equal 11, first.value
    assert_equal 11, second.value
    refute_same first, second
  end

  def test_unique_and_shared_ownership
    assert_equal 20, Classes.make_owned_child(20).value
    assert_equal 21, Classes.make_shared_child(21).value
  end

  def test_member_value_survives_compaction
    holder = holder_with_only_native_reference
    compacted { holder }
    assert_equal "held by C++", holder.value
  end

  def test_unregistered_type_error_is_actionable
    error = assert_raises(TypeError) { Classes.consume_unregistered(Object.new) }
    assert_match(/def_class<T>/, error.message)
  end

  private

  def holder_with_only_native_reference
    Classes::RubyHolder.new(String.new("held by C++"))
  end
end
