# frozen_string_literal: true

require_relative "helper"

class PackagingTest < Minitest::Test
  ROOT = File.expand_path("../..", __dir__)

  def test_gem_contains_headers_build_helpers_and_generator_assets
    specification = Gem::Specification.load(File.join(ROOT, "rbxx.gemspec"))

    assert_includes specification.files, "include/rbxx/rbxx.hpp"
    assert_includes specification.files, "single_include/rbxx/rbxx.hpp"
    assert_includes specification.files, "lib/rbxx/mkmf.rb"
    assert_includes specification.files, "ext-cmake/rbxx-config.cmake"
    assert_includes specification.files, "templates/extension.cpp.erb"
    assert_includes specification.executables, "rbxx"
  end
end
