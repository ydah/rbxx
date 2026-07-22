# frozen_string_literal: true

require_relative "lib/rbxx/version"

Gem::Specification.new do |spec|
  spec.name = "rbxx"
  spec.version = Rbxx::VERSION
  spec.authors = ["Yudai Takada"]
  spec.email = ["t.yudai92@gmail.com"]

  spec.summary = "Modern C++20 bindings for CRuby native extensions"
  spec.description = "rbxx is a header-only C++20 binding library focused on safe exception, " \
                     "GC, and ownership boundaries."
  spec.homepage = "https://github.com/ydah/rbxx"
  spec.license = "MIT"
  spec.required_ruby_version = ">= 3.1.0"
  spec.metadata["homepage_uri"] = spec.homepage
  spec.metadata["source_code_uri"] = spec.homepage
  spec.metadata["changelog_uri"] = "#{spec.homepage}/blob/main/CHANGELOG.md"
  spec.metadata["rubygems_mfa_required"] = "true"

  spec.files = Dir.chdir(__dir__) do
    `git ls-files -z`.split("\x0").reject do |file|
      file.start_with?(*%w[.github/ .idea/ spec/]) || file == "Gemfile"
    end
  end
  spec.bindir = "exe"
  spec.executables = spec.files.grep(%r{\Aexe/}) { |file| File.basename(file) }
  spec.require_paths = ["lib"]
end
