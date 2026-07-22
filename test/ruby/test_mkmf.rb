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

  def test_cmake_separates_the_compiler_from_its_flags
    configure, = Rbxx::Mkmf.cmake_commands("/project/ext", "cmake-build",
                                           cxx: "clang++ -std=gnu++11",
                                           platform: "arm64-darwin")

    assert_includes configure, "-DCMAKE_CXX_COMPILER=clang++"
    assert_includes configure, "-DCMAKE_CXX_FLAGS_INIT=-std=gnu++11"
  end

  def test_cmake_does_not_pass_the_parent_make_to_mingw_make
    configure, build = Rbxx::Mkmf.cmake_commands("/project/ext", "cmake-build",
                                                 cxx: "g++ -std=gnu++11",
                                                 platform: "x64-mingw-ucrt")
    makefile = Rbxx::Mkmf.cmake_makefile(configure, build, "cmake-build", "demo.so",
                                         windows: true)

    assert_equal "unexport MAKE", makefile.lines.first.chomp
    assert_equal ["-G", "MinGW Makefiles"], configure.last(2)
    assert_includes makefile, "-Drbxx_DIR=#{Rbxx::Mkmf.cmake_config_dir}"
    assert_includes makefile, '"MinGW Makefiles"'
    refute_includes makefile, "\\="
  end
end
