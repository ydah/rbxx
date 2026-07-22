# 0002: Object pin storage

- Status: Accepted
- Date: 2026-07-22

## Context

Releasing slots from a hidden backing Array with a free list would call the Ruby Array API from a
destructor, introducing the risk of a Ruby exception. The alternative was to use shared cells
registered with `rb_gc_register_address`.

## Decision

Register a shared cell that stores the `VALUE`. Copies share the cell, while moves transfer
ownership.

## Consequences

Compaction updates the pinned value, and releasing the final handle invokes only non-raising APIs.
Array slot management is unnecessary, and the observable copy and move semantics match the
design.
