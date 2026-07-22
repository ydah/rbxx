# Source and precompiled distribution

Generate a release-ready source gem with `rbxx new NAME`. Use `--cmake` when the wrapped library has
its own CMake dependency graph. The source gem must always retain `spec.extensions`; precompiled gems
are an installation optimization, not the only installation route.

`Rbxx::RakeTasks` defines these rake-compiler-dock targets:

- `x86_64-linux`
- `aarch64-linux`
- `x86_64-darwin`
- `arm64-darwin`
- `x64-mingw-ucrt`

Start with `bundle exec rake precompiled:dry_run`. Build one artifact with
`bundle exec rake precompiled:x86_64-linux`, or all with `precompiled:all`. Each Ruby minor version
needs its own extension binary in the native gem.

CI should test the source build on all supported operating systems, build at least x86_64 Linux in
rake-compiler-dock, install each produced gem into a clean Ruby, and run a smoke require. Artifact
publication is intentionally outside rbxx automation: inspect filenames and dependencies before a
human runs `gem push`.

See [the complete example](../../examples/precompiled-gem).
