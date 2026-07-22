# frozen_string_literal: true

require_relative "helper"
require "rbxx/mkmf"

class MkmfTest < Minitest::Test
  def test_gnu_and_clang_configuration
    flags = Rbxx::Mkmf.compiler_flags("/project/include", debug: true, sanitize: true,
                                                          compiler: :gnu)

    assert_includes flags[:cppflags], "-I/project/include"
    assert_includes flags[:cppflags], "-DRBXX_DEBUG=1"
    assert_includes flags[:cxxflags], "-std=c++20"
    assert_includes flags[:cxxflags], "-fvisibility=hidden"
    assert_includes flags[:cxxflags], "-fsanitize=address,undefined"
    assert_includes flags[:ldflags], "-fsanitize=address,undefined"
  end

  def test_msvc_configuration
    flags = Rbxx::Mkmf.compiler_flags("C:/project/include", debug: true, sanitize: false,
                                                            compiler: :msvc)

    assert_includes flags[:cppflags], '/I"C:/project/include"'
    assert_includes flags[:cppflags], "/DRBXX_DEBUG=1"
    assert_includes flags[:cxxflags], "/std:c++20"
    assert_includes flags[:cxxflags], "/EHsc"
    assert_includes flags[:cxxflags], "/utf-8"
  end
end
