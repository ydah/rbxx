# frozen_string_literal: true

require "bundler/gem_tasks"
require "fileutils"
require "rbconfig"
require "rake/clean"
require "rake/testtask"
require "shellwords"

ROOT = File.expand_path(__dir__)
TEST_BUILD_ROOT = File.join(ROOT, "tmp", "test_extensions")
TEST_SOURCES = FileList[File.join(ROOT, "test", "cpp", "*.cpp")]

CLEAN.include(TEST_BUILD_ROOT)
CLOBBER.include(File.join(ROOT, "single_include", "rbxx", "rbxx.hpp"))

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
