# 0003: Method dispatch strategy

- Status: Accepted
- Date: 2026-07-22

## Context

Static trampolines for each callable type make collision avoidance complex for function pointers
of the same type. A single `(owner, ID)` registry provides a simpler implementation and exception
boundary, but the initial registry path took 18.6 times as long as handwritten C for a
zero-argument method returning an integer and did not satisfy the G2 performance goal.

## Decision

Use the registry for keyword arguments, blocks, and overloads. Protect definitions with a mutex
and treat the table as read-only once invocation begins. Use a fixed-arity slot for a single
zero-argument member, and let `def<&T::method>("name")` embed the member pointer in the trampoline
to provide an explicit shortest path.

## Consequences

All general callable types retain the same dispatch path. Adding an overload with the same name
redefines the Ruby method to use the generic trampoline, so its semantics remain unchanged. The
compile-time path takes 1.02 times as long as handwritten C and delivers roughly three times
Rice's throughput, while the additional implementation complexity remains limited to the
performance-critical path.
