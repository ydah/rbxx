# frozen_string_literal: true

require "bundler/gem_tasks"
require "fileutils"
require "rbconfig"
require "rake/clean"
require "rake/testtask"
require "open3"
require "shellwords"

ROOT = File.expand_path(__dir__)
TEST_BUILD_ROOT = File.join(ROOT, "tmp", "test_extensions")
TEST_SOURCES = FileList[File.join(ROOT, "test", "cpp", "*.cpp")]

CLEAN.include(TEST_BUILD_ROOT)

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
          require "mkmf"
          $CPPFLAGS = "-I#{File.join(ROOT, 'include')}" + " " + $CPPFLAGS
          $CXXFLAGS = "-std=c++20 -Wall -Wextra" + " " + $CXXFLAGS unless /mswin|msvc/ =~ RUBY_PLATFORM
          $CXXFLAGS = "/std:c++20 /EHsc /utf-8" + " " + $CXXFLAGS if /mswin|msvc/ =~ RUBY_PLATFORM
          create_makefile("#{name}")
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
      includes = [File.join(ROOT, "include"), RbConfig::CONFIG.fetch("rubyhdrdir"),
                  RbConfig::CONFIG.fetch("rubyarchhdrdir")]
      command = [RbConfig::CONFIG.fetch("CXX"), "-std=c++20", "-fsyntax-only",
                 *includes.map { |path| "-I#{path}" }, source]
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
    FileUtils.mkdir_p(build_dir)
    File.write(File.join(build_dir, "single_header_smoke.cpp"), <<~CPP)
      #include <rbxx/rbxx.hpp>
      extern "C" void Init_single_header_smoke() {}
    CPP
    File.write(File.join(build_dir, "extconf.rb"), <<~RUBY)
      require "mkmf"
      $CPPFLAGS = "-I#{File.join(ROOT, 'single_include')} " + $CPPFLAGS
      $CXXFLAGS = "-std=c++20 " + $CXXFLAGS unless /mswin|msvc/ =~ RUBY_PLATFORM
      $CXXFLAGS = "/std:c++20 /EHsc /utf-8 " + $CXXFLAGS if /mswin|msvc/ =~ RUBY_PLATFORM
      create_makefile("single_header_smoke")
    RUBY
    Dir.chdir(build_dir) do
      sh RbConfig.ruby, "extconf.rb" unless File.exist?("Makefile")
      sh RbConfig::CONFIG.fetch("MAKE", "make")
    end
  end
end

desc "Generate the rbxx single header"
task amalgamate: "amalgamate:generate"
