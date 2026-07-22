# Ruby callback

This example converts a Ruby `Proc` to `std::function<int(int)>` and calls it from C++.

```sh
cd examples/callback
ruby extconf.rb && make
ruby run.rb
```

Callbacks execute with the GVL. Do not capture one inside work passed to `rbxx::nogvl`.
