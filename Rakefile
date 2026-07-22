# frozen_string_literal: true

require "bundler/gem_tasks"
require "fileutils"
require "rbconfig"
require "rake/clean"
require "rake/testtask"
require "open3"
require "shellwords"
require "benchmark"
require_relative "lib/rbxx/rake_tasks"

ROOT = File.expand_path(__dir__)
TEST_BUILD_ROOT = File.join(ROOT, "tmp", "test_extensions")
TEST_SOURCES = FileList[File.join(ROOT, "test", "cpp", "*.cpp")]
BENCH_EXTENSION_ROOT = File.join(ROOT, "bench", "ext")

CLEAN.include(TEST_BUILD_ROOT)
CLEAN.include(File.join(BENCH_EXTENSION_ROOT, "**", "Makefile"))
CLEAN.include(File.join(BENCH_EXTENSION_ROOT, "**", "*.{o,bundle,so,dll}"))

def syntax_command(source)
  includes = [File.join(ROOT, "include"), RbConfig::CONFIG.fetch("rubyhdrdir"),
              RbConfig::CONFIG.fetch("rubyarchhdrdir")]
  compiler = Shellwords.split(RbConfig::CONFIG.fetch("CXX"))
  if File.basename(compiler.first).match?(/\Acl(?:\.exe)?\z/i)
    compiler + ["/std:c++20", "/Zs", *includes.map { |path| %(/I"#{path}") }, source]
  else
    compiler + ["-std=c++20", "-fsyntax-only", *includes.map { |path| "-I#{path}" }, source]
  end
end

namespace :compile do
  desc "Build all native test extensions"
  task :test do
    TEST_SOURCES.each do |source|
      name = File.basename(source, ".cpp")
      build_dir = File.join(TEST_BUILD_ROOT, name)
      FileUtils.mkdir_p(build_dir)
      FileUtils.cp(source, File.join(build_dir, "#{name}.cpp"))
      File.write(
        File.join(build_dir, "extconf.rb"),
        <<~RUBY
          # frozen_string_literal: true
          $LOAD_PATH.unshift(#{File.join(ROOT, 'lib').inspect})
          require "rbxx/mkmf"
          create_rbxx_makefile("#{name}", include_path: #{File.join(ROOT, 'include').inspect})
        RUBY
      )

      Dir.chdir(build_dir) do
        ruby = RbConfig.ruby.shellescape
        sh "#{ruby} extconf.rb" unless File.exist?("Makefile")
        sh RbConfig::CONFIG.fetch("MAKE", "make")
      end
    end
  end
end

Rake::TestTask.new(:test) do |task|
  task.libs << "lib"
  task.libs << TEST_BUILD_ROOT
  task.pattern = "test/ruby/test_*.rb"
  task.warning = true
end

namespace :test do
  desc "Run the test suite with GC.stress enabled"
  task gc_stress: ["compile:test"] do
    loader = 'Dir["test/ruby/test_*.rb"].sort.each { |file| require File.expand_path(file) }'
    sh({ "RBXX_GC_STRESS" => "1" }, RbConfig.ruby, "-Ilib", "-I#{TEST_BUILD_ROOT}",
       "-Itest/ruby", "-e", loader)
  end

  desc "Verify expected compile-time diagnostics"
  task :compile_fail do
    FileList[File.join(ROOT, "test", "compile_fail", "*.cpp")].each do |source|
      expected = File.foreach(source).first.to_s.delete_prefix("// EXPECT:").strip
      command = syntax_command(source)
      _stdout, stderr, status = Open3.capture3(*command)
      raise "#{source} unexpectedly compiled" if status.success?
      raise "#{source} did not contain expected diagnostic: #{expected}\n#{stderr}" unless stderr.include?(expected)
    end
  end
end

desc "Check Ruby and C++ formatting"
task :lint do
  sh "bundle exec rubocop --force-exclusion lib Rakefile rbxx.gemspec test/ruby"

  formatter = ENV.fetch("CLANG_FORMAT", "clang-format")
  if system("command -v #{formatter.shellescape} >/dev/null 2>&1")
    sources = FileList["include/**/*.hpp", "test/cpp/*.cpp"]
    sh "#{formatter.shellescape} --dry-run --Werror #{sources.map(&:shellescape).join(' ')}" unless sources.empty?
  else
    warn "clang-format not found; skipping C++ format check"
  end
end

task default: ["compile:test", :test]

namespace :amalgamate do
  desc "Generate the rbxx single header"
  task :generate do
    sh RbConfig.ruby, File.join(ROOT, "scripts", "amalgamate.rb")
  end

  desc "Verify that the committed single header is current"
  task :check do
    sh RbConfig.ruby, File.join(ROOT, "scripts", "amalgamate.rb"), "--check"
  end

  desc "Compile an extension using only the generated single header"
  task smoke: [:generate] do
    build_dir = File.join(ROOT, "tmp", "amalgamate_smoke")
    FileUtils.rm_rf(build_dir)
    FileUtils.mkdir_p(build_dir)
    File.write(File.join(build_dir, "single_header_smoke.cpp"), <<~CPP)
      #include <rbxx/rbxx.hpp>
      extern "C" RBXX_EXPORT void Init_single_header_smoke() {}
    CPP
    File.write(File.join(build_dir, "extconf.rb"), <<~RUBY)
      require "mkmf"
      $CPPFLAGS = "-I#{File.join(ROOT, 'single_include')} " + $CPPFLAGS
      $CXXFLAGS = "-std=c++20 " + $CXXFLAGS unless /mswin|msvc/ =~ RUBY_PLATFORM
      $CXXFLAGS = "/std:c++20 /EHsc /utf-8 " + $CXXFLAGS if /mswin|msvc/ =~ RUBY_PLATFORM
      create_makefile("single_header_smoke")
    RUBY
    Dir.chdir(build_dir) do
      sh RbConfig.ruby, "extconf.rb"
      sh RbConfig::CONFIG.fetch("MAKE", "make")
    end
  end
end

desc "Generate the rbxx single header"
task amalgamate: "amalgamate:generate"

Rbxx::RakeTasks.install(command: "bundle install && rake amalgamate:smoke")

namespace :bench do
  desc "Build the hand-written C, rbxx, and Rice benchmark extensions"
  task :build do
    %w[c rbxx rice].each do |implementation|
      directory = File.join(BENCH_EXTENSION_ROOT, implementation)
      Dir.chdir(directory) do
        sh RbConfig::CONFIG.fetch("MAKE", "make"), "clean" if File.exist?("Makefile")
        sh RbConfig.ruby, "extconf.rb"
        sh RbConfig::CONFIG.fetch("MAKE", "make")
      end
    end
  end

  desc "Run the native binding comparison benchmark"
  task run: :build do
    paths = %w[c rbxx rice].map { |name| File.join(BENCH_EXTENSION_ROOT, name) }
    sh({ "RUBYLIB" => paths.join(File::PATH_SEPARATOR) }, RbConfig.ruby,
       File.join(ROOT, "bench", "bindings.rb"))
  end

  desc "Measure syntax-only compilation of a representative binding"
  task :compile_time do
    source = File.join(ROOT, "test", "cpp", "arguments.cpp")
    elapsed = Benchmark.realtime do
      _stdout, stderr, status = Open3.capture3(*syntax_command(source))
      raise "representative binding did not compile:\n#{stderr}" unless status.success?
    end
    puts format("representative binding syntax check: %.3f s", elapsed)
  end
end

desc "Build the public C++ API reference"
task :docs do
  sh "doxygen", File.join(ROOT, "Doxyfile")
end

namespace :examples do
  desc "Build and execute all four example projects"
  task :smoke do
    %w[class-binding callback].each do |name|
      directory = File.join(ROOT, "examples", name)
      Dir.chdir(directory) do
        sh RbConfig.ruby, "extconf.rb"
        sh RbConfig::CONFIG.fetch("MAKE", "make")
        sh RbConfig.ruby, "run.rb"
      end
    end

    cmake_build = File.join(ROOT, "tmp", "cmake-example")
    sh "cmake", "-S", File.join(ROOT, "examples", "cmake-demo"), "-B", cmake_build
    sh "cmake", "--build", cmake_build
    sh RbConfig.ruby, "-I#{cmake_build}", "-rcmake_demo", "-e",
       "abort unless CmakeDemo.square(9) == 81 && !CmakeDemo.zlib_version.empty?"

    project = File.join(ROOT, "examples", "precompiled-gem")
    extension = File.join(project, "ext", "precompiled_demo")
    Dir.chdir(extension) do
      sh RbConfig.ruby, "-I#{File.join(ROOT, 'lib')}", "extconf.rb"
      sh RbConfig::CONFIG.fetch("MAKE", "make")
    end
    sh RbConfig.ruby, "-I#{extension}", "-rprecompiled_demo", "-e",
       "abort unless PrecompiledDemo.multiply(6, 7) == 42"
  end
end
