# frozen_string_literal: true

$LOAD_PATH.unshift(File.expand_path("../../lib", __dir__))
require "rbxx/mkmf"

create_rbxx_makefile("callback")
