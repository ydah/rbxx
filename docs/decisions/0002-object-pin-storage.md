# ADR 0002: Object pin storage

- Context: Releasing slots from the proposed backing Array would call the Ruby Array API from a
  destructor, introducing the risk of a Ruby exception.
- Options: A hidden Array with a free list / shared cells registered with
  `rb_gc_register_address`.
- Decision: Register a shared cell that stores the `VALUE`. Copies share the cell, while moves
  transfer ownership.
- Consequences: Compaction updates the pinned value, and releasing the final handle invokes only
  non-raising APIs.
- Consequences: Array slot management is unnecessary, and the observable copy and move semantics
  match the design.
