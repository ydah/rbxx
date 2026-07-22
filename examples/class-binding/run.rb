# frozen_string_literal: true

require_relative "class_binding"

counter = ClassBinding::Counter.new(start: 10)
counter.add(5)
counter.subtract(3)
raise "unexpected value" unless counter.value == 12 && counter.doubled == 24

counter.reset
puts counter.value
