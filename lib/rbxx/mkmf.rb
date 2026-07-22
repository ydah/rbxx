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
      windows = RUBY_PLATFORM.include?("mingw")
      makefile = cmake_makefile(configure, build, build_dir, extension, windows:)
      File.write("Makefile", makefile)
    end

    def cmake_commands(source_dir, build_dir, cxx: RbConfig::CONFIG.fetch("CXX"),
                       platform: RUBY_PLATFORM)
      compiler, *compiler_flags = Shellwords.split(cxx)
      raise ArgumentError, "rbxx: Ruby C++ compiler is not configured" unless compiler

      configure = ["cmake", "-S", File.expand_path(source_dir), "-B", build_dir,
                   "-Drbxx_DIR=#{cmake_config_dir}", "-DRuby_EXECUTABLE=#{RbConfig.ruby}",
                   "-DCMAKE_CXX_COMPILER=#{compiler}"]
      configure << "-DCMAKE_CXX_FLAGS_INIT=#{compiler_flags.join(' ')}" unless compiler_flags.empty?
      build = ["cmake", "--build", build_dir, "--config", "Release"]

      configure.push("-G", "MinGW Makefiles") if platform.include?("mingw")

      [configure, build]
    end

    def cmake_makefile(configure, build, build_dir, extension, windows: false)
      built_extension = File.join(build_dir, extension)
      install_copy = if windows
                       shell_join(["cmake", "-E", "copy", extension,
                                   "$(sitearchdir)/#{extension}"], windows: true)
                     else
                       "cmake -E copy #{Shellwords.escape(extension)} $(sitearchdir)/#{extension}"
                     end
      lines = [
        ".PHONY: all clean install", "", "all:", "\t#{shell_join(configure, windows:)}",
        "\t#{shell_join(build, windows:)}",
        "\t#{shell_join(['cmake', '-E', 'copy', built_extension, extension], windows:)}", "",
        "install: all", "\tcmake -E make_directory $(sitearchdir)",
        "\t#{install_copy}", "", "clean:",
        "\t#{shell_join(['cmake', '-E', 'remove_directory', build_dir], windows:)}",
        "\t#{shell_join(['cmake', '-E', 'rm', '-f', extension], windows:)}", ""
      ]
      lines.unshift("unexport MAKE", "") if windows
      lines.join("\n")
    end

    def shell_join(arguments, windows: false)
      return Shellwords.join(arguments) unless windows

      arguments.map { |argument| windows_shell_escape(argument) }.join(" ")
    end

    def windows_shell_escape(argument)
      value = argument.to_s
      return value if value.match?(%r{\A[A-Za-z0-9_+.,:/\\=@-]+\z})

      escaped = value.gsub(/(\\*)"/) { "#{Regexp.last_match(1) * 2}\\\"" }
      %("#{escaped.sub(/(\\+)\z/) { |slashes| slashes * 2 }}")
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
      optimization = debug ? "-O0 -g" : RbConfig::CONFIG.fetch("optflags", "-O2")
      cxxflags = " #{optimization} -std=c++20 -Wall -Wextra -fvisibility=hidden"
      cxxflags += " -fsanitize=address,undefined -fno-omit-frame-pointer" if sanitize
      ldflags = sanitize ? " -fsanitize=address,undefined" : ""
      { cppflags:, cxxflags:, ldflags: }
    end

    def msvc_flags(include_path, debug:)
      cppflags = %( /I"#{include_path}")
      cppflags += " /DRBXX_DEBUG=1" if debug
      optimization = debug ? "/Od /Zi" : "/O2"
      { cppflags:, cxxflags: " #{optimization} /std:c++20 /EHsc /utf-8 /W4", ldflags: "" }
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
