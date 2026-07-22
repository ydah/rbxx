# frozen_string_literal: true

require "mkmf-rice"

$CXXFLAGS = "#{RbConfig::CONFIG.fetch('optflags', '-O2')} #{$CXXFLAGS}"
create_makefile("bench_rice")
