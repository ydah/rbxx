# ADR 0003: Initial method dispatch

- Context: Static trampolines for each callable type make collision avoidance complex for function
  pointers of the same type.
- Options: Unique trampolines with a fallback / a single `(owner, ID)` registry.
- Decision: Use the registry approach permitted by the design for the initial Phase 3
  implementation.
- Decision: Protect definitions with a mutex and treat the table as read-only once invocation
  begins.
- Consequences: The implementation and exception boundary are simpler, and all callable types use
  the same path.
- Consequences: Reconsider unique trampolines only if the Phase 9 benchmark misses its target.

## Phase 9 follow-up

- Measurement: The initial registry path took 18.6 times as long as handwritten C for a
  zero-argument method returning an integer, so it did not satisfy G2.
- Decision: Retain the generic registry for keyword arguments, blocks, and overloads, while using a
  fixed-arity slot for a single zero-argument member.
- Decision: `def<&T::method>("name")` also embeds the member pointer in the trampoline, providing an
  explicit API for the shortest path.
- Compatibility: Adding an overload with the same name redefines the Ruby method to use the generic
  trampoline, so the semantics remain unchanged.
- Result: The compile-time path takes 1.02 times as long as handwritten C and delivers roughly three
  times Rice's throughput.
