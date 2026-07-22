# frozen_string_literal: true

require "erb"
require "fileutils"
require "optparse"

module Rbxx
  # Command-line project generator for source and CMake-based native gems.
  class CLI
    TEMPLATE_ROOT = File.expand_path("../../templates", __dir__)

    def self.start(arguments, out: $stdout, err: $stderr)
      new(arguments.dup, out:, err:).run
    rescue OptionParser::ParseError, ArgumentError => e
      err.puts(e.message)
      1
    end

    def initialize(arguments, out:, err:)
      @arguments = arguments
      @out = out
      @err = err
    end

    def run
      command = @arguments.shift
      return create_project if command == "new"

      raise ArgumentError, "usage: rbxx new NAME [--cmake]"
    end

    private

    def create_project
      options = parse_options
      name = @arguments.shift
      validate_name(name)
      raise ArgumentError, "unexpected arguments: #{@arguments.join(' ')}" unless @arguments.empty?

      project = Project.new(name:, cmake: options[:cmake], rbxx_path: options[:rbxx_path])
      project.generate
      @out.puts("Created #{project.destination}")
      0
    end

    def parse_options
      options = { cmake: false, rbxx_path: nil }
      OptionParser.new do |parser|
        parser.on("--cmake") { options[:cmake] = true }
        parser.on("--rbxx-path PATH") { |path| options[:rbxx_path] = File.expand_path(path) }
      end.parse!(@arguments)
      options
    end

    def validate_name(name)
      return if name&.match?(/\A[a-z][a-z0-9_]*\z/)

      raise ArgumentError, "NAME must use lowercase letters, digits, and underscores"
    end

    # Rendering context for one generated gem project.
    class Project
      attr_reader :destination

      def initialize(name:, cmake:, rbxx_path:)
        @name = name
        @cmake = cmake
        @rbxx_path = rbxx_path
        @destination = File.expand_path(name)
      end

      def generate
        raise ArgumentError, "destination already exists: #{destination}" if File.exist?(destination)

        templates.each do |source, relative_destination|
          render(source, File.join(destination, relative_destination))
        end
      end

      def class_name
        @name.split("_").map(&:capitalize).join
      end

      def rbxx_gem_line
        return %(gem "rbxx", path: #{@rbxx_path.inspect}) if @rbxx_path

        %(gem "rbxx", "~> 0.1")
      end

      private

      attr_reader :name

      def templates
        common_templates.merge(build_templates)
      end

      def common_templates
        {
          "Gemfile.erb" => "Gemfile",
          "gemspec.erb" => "#{name}.gemspec",
          "Rakefile.erb" => "Rakefile",
          "README.md.erb" => "README.md",
          "extension.cpp.erb" => "ext/#{name}/#{name}.cpp",
          "lib.rb.erb" => "lib/#{name}.rb",
          "version.rb.erb" => "lib/#{name}/version.rb",
          "test.rb.erb" => "test/test_#{name}.rb",
          "ci.yml.erb" => ".github/workflows/ci.yml",
          "release.yml.erb" => ".github/workflows/release.yml"
        }
      end

      def build_templates
        extconf = @cmake ? "extconf_cmake.rb.erb" : "extconf.rb.erb"
        result = { extconf => "ext/#{name}/extconf.rb" }
        result["CMakeLists.txt.erb"] = "ext/#{name}/CMakeLists.txt" if @cmake
        result
      end

      def render(source, target)
        FileUtils.mkdir_p(File.dirname(target))
        template = File.read(File.join(TEMPLATE_ROOT, source))
        File.write(target, ERB.new(template, trim_mode: "-").result(binding))
      end
    end
  end
end
