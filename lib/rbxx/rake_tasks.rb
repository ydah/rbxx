# frozen_string_literal: true

require "rake"

module Rbxx
  # Reusable rake-compiler-dock tasks for the supported native gem platforms.
  module RakeTasks
    extend Rake::DSL

    PLATFORMS = %w[
      x86_64-linux
      aarch64-linux
      x86_64-darwin
      arm64-darwin
      x64-mingw-ucrt
    ].freeze

    module_function

    def install(namespace: :precompiled, command: "bundle install && rake native:%<platform>s gem")
      namespace(namespace) do
        desc "Print precompiled gem build commands without starting containers"
        task :dry_run do
          PLATFORMS.each { |platform| puts build_description(platform, command) }
        end

        PLATFORMS.each do |platform|
          desc "Build the #{platform} native gem with rake-compiler-dock"
          task platform do
            require "rake_compiler_dock"
            RakeCompilerDock.sh(format(command, platform:), platform:)
          end
        end

        desc "Build native gems for all supported platforms"
        task all: PLATFORMS
      end
    end

    def build_description(platform, command)
      "RCD_PLATFORM=#{platform} #{format(command, platform:)}"
    end
  end
end
