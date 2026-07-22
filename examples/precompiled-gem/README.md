# Precompiled gem example

This directory is a complete source gem with a native extension, tests, and a release workflow.

```sh
bundle install
bundle exec rake test
bundle exec rake precompiled:dry_run
```

`precompiled:all` uses rake-compiler-dock for `x86_64-linux`, `aarch64-linux`,
`x86_64-darwin`, `arm64-darwin`, and `x64-mingw-ucrt`. The source extension remains in the gem,
so unsupported platforms can compile it during installation. Publishing still requires a human to
inspect the artifacts and run `gem push`.
