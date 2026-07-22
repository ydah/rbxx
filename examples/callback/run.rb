# frozen_string_literal: true

require_relative "callback"

double = ->(value) { value * 2 }
p CallbackExample.transform([1, 2, 3], double)
