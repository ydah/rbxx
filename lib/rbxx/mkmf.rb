# frozen_string_literal: true

require "mkmf"
require "rbconfig"
require "shellwords"

module Rbxx
  # Shared mkmf and CMake configuration for extensions built with rbxx.
  module Mkmf
    module_function

    def include_dir
      File.expand_path("../../include", __dir__)
    end

    def cmake_config_dir
      File.expand_path("../../ext-cmake", __dir__)
    end

    def create_makefile(target, include_path: include_dir, debug: debug?, sanitize: sanitize?)
      configure_cpp_flags(include_path, debug:, sanitize:)
      super_create_makefile(target)
    end

    def create_cmake_makefile(target, source_dir: __dir__, build_dir: "cmake-build")
      target_name = File.basename(target)
      configure, build = cmake_commands(source_dir, build_dir)
      extension = "#{target_name}.#{RbConfig::CONFIG.fetch('DLEXT')}"
      File.write("Makefile", cmake_makefile(configure, build, build_dir, extension))
    end

    def cmake_commands(source_dir, build_dir)
      configure = ["cmake", "-S", File.expand_path(source_dir), "-B", build_dir,
                   "-Drbxx_DIR=#{cmake_config_dir}", "-DRuby_EXECUTABLE=#{RbConfig.ruby}",
                   "-DCMAKE_CXX_COMPILER=#{RbConfig::CONFIG.fetch('CXX')}"]
      configure.push("-G", "MinGW Makefiles") if RUBY_PLATFORM.include?("mingw")
      build = ["cmake", "--build", build_dir, "--config", "Release"]
      [configure, build]
    end

    def cmake_makefile(configure, build, build_dir, extension)
      built_extension = File.join(build_dir, extension)
      [
        ".PHONY: all clean install", "", "all:", "\t#{Shellwords.join(configure)}",
        "\t#{Shellwords.join(build)}",
        "\tcmake -E copy #{Shellwords.escape(built_extension)} #{Shellwords.escape(extension)}", "",
        "install: all", "\tcmake -E make_directory $(sitearchdir)",
        "\tcmake -E copy #{Shellwords.escape(extension)} $(sitearchdir)/#{extension}", "", "clean:",
        "\tcmake -E remove_directory #{Shellwords.escape(build_dir)}",
        "\tcmake -E rm -f #{Shellwords.escape(extension)}", ""
      ].join("\n")
    end

    def debug?
      ENV["RBXX_DEBUG"] == "1"
    end

    def sanitize?
      ENV["RBXX_SANITIZE"] == "1"
    end

    def configure_cpp_flags(include_path, debug:, sanitize:)
      compiler_flags(include_path, debug:, sanitize:).each do |name, flags|
        append_global(name, flags)
      end
    end

    def compiler_flags(include_path, debug:, sanitize:, compiler: msvc? ? :msvc : :gnu)
      return msvc_flags(include_path, debug:) if compiler == :msvc

      cppflags = " -I#{Shellwords.escape(include_path)}"
      cppflags += " -DRBXX_DEBUG=1" if debug
      cxxflags = " -std=c++20 -Wall -Wextra -fvisibility=hidden"
      cxxflags += " -fsanitize=address,undefined -fno-omit-frame-pointer" if sanitize
      ldflags = sanitize ? " -fsanitize=address,undefined" : ""
      { cppflags:, cxxflags:, ldflags: }
    end

    def msvc_flags(include_path, debug:)
      cppflags = %( /I"#{include_path}")
      cppflags += " /DRBXX_DEBUG=1" if debug
      { cppflags:, cxxflags: " /std:c++20 /EHsc /utf-8 /W4", ldflags: "" }
    end

    # mkmf exposes compiler configuration exclusively through these globals.
    # rubocop:disable Style/GlobalVars
    def append_global(name, suffix)
      $CPPFLAGS = $CPPFLAGS.to_s + suffix if name == :cppflags
      $CXXFLAGS = $CXXFLAGS.to_s + suffix if name == :cxxflags
      $LDFLAGS = $LDFLAGS.to_s + suffix if name == :ldflags
    end
    # rubocop:enable Style/GlobalVars

    def msvc?
      compiler = File.basename(RbConfig::CONFIG.fetch("CC", "").split.first.to_s)
      compiler.match?(/\Acl(?:\.exe)?\z/i) || RUBY_PLATFORM.include?("mswin")
    end

    def super_create_makefile(target)
      TOPLEVEL_BINDING.receiver.send(:create_makefile, target)
    end
  end
end

def create_rbxx_makefile(target, **options)
  Rbxx::Mkmf.create_makefile(target, **options)
end

def create_rbxx_cmake_makefile(target, **options)
  Rbxx::Mkmf.create_cmake_makefile(target, **options)
end
