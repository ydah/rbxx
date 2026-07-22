# ADR 0001: Test extension module name

- Context: The work instructions specify `rbxxTest`, but CRuby constant names must begin with an
  uppercase letter.
- Options: Preserve the invalid name / standardize on the valid `RbxxTest` name.
- Decision: Use `RbxxTest` as the test extension namespace in both C++ and Ruby.
- Consequences: The name differs from the work instructions only in capitalization and loads
  correctly on every supported OS and Ruby version.
