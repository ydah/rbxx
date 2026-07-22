# frozen_string_literal: true

require_relative "lib/precompiled_demo/version"

Gem::Specification.new do |spec|
  spec.name = "precompiled_demo"
  spec.version = PrecompiledDemo::VERSION
  spec.authors = ["Example Maintainer"]
  spec.summary = "Complete rbxx source and precompiled gem example"
  spec.license = "MIT"
  spec.required_ruby_version = ">= 3.1"
  spec.files = Dir["{ext,lib}/**/*", "README.md"]
  spec.require_paths = ["lib"]
  spec.extensions = ["ext/precompiled_demo/extconf.rb"]
  spec.add_dependency "rbxx", "~> 0.1"
end
