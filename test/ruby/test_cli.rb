# frozen_string_literal: true

require_relative "helper"
require "bundler"
require "open3"
require "rbxx/cli"
require "stringio"
require "tmpdir"

class CliTest < Minitest::Test
  ROOT = File.expand_path("../..", __dir__)

  def test_generates_and_builds_mkmf_project
    generate_and_test("generated_demo")
  end

  def test_generates_and_builds_cmake_project
    generate_and_test("generated_cmake", cmake: true)
  end

  def test_rejects_an_unsafe_project_name
    output = StringIO.new
    status = Rbxx::CLI.start(["new", "../unsafe"], out: output, err: output)

    assert_equal 1, status
    assert_match(/lowercase letters/, output.string)
  end

  private

  def generate_and_test(name, cmake: false)
    Dir.mktmpdir("rbxx-cli") do |directory|
      project = generate_project(directory, name, cmake:)
      run!("bundle", "install", "--local", chdir: project)
      run!("bundle", "exec", "rake", "test", chdir: project)
      run!("bundle", "exec", "rake", "precompiled:dry_run", chdir: project)
    end
  end

  def generate_project(directory, name, cmake:)
    arguments = ["new", name, "--rbxx-path", ROOT]
    arguments << "--cmake" if cmake
    output = StringIO.new
    status = Dir.chdir(directory) { Rbxx::CLI.start(arguments, out: output, err: output) }
    assert_equal 0, status, output.string
    File.join(directory, name)
  end

  def run!(*command, chdir:)
    gem_home = File.join(chdir, ".gem")
    environment = {
      "BUNDLE_APP_CONFIG" => File.join(chdir, ".bundle"),
      "BUNDLE_BIN" => File.join(chdir, ".bundle", "bin"),
      "BUNDLE_GEMFILE" => File.join(chdir, "Gemfile"),
      "BUNDLE_LOCKFILE" => nil,
      "BUNDLE_PATH" => nil,
      "GEM_HOME" => gem_home,
      "GEM_PATH" => ([gem_home] + Gem.path).join(File::PATH_SEPARATOR)
    }
    stdout, stderr, status = Bundler.with_unbundled_env do
      Open3.capture3(environment, *command, chdir:)
    end
    assert status.success?, <<~MESSAGE
      #{command.join(' ')} failed in #{chdir}
      #{stdout}
      #{stderr}
    MESSAGE
  end
end
