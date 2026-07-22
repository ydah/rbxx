#pragma once

#include <rbxx/value.hpp>

#include <memory>
#include <utility>

namespace rbxx {
namespace detail {

class object_cell {
public:
  explicit object_cell(VALUE initial) noexcept : stored_(initial) {
    rb_gc_register_address(&stored_);
  }

  object_cell(const object_cell&) = delete;
  object_cell& operator=(const object_cell&) = delete;

  ~object_cell() noexcept { rb_gc_unregister_address(&stored_); }

  [[nodiscard]] VALUE raw() const noexcept { return stored_; }

private:
  VALUE stored_;
};

} // namespace detail

/// @brief A copyable, movable RAII handle that pins a Ruby object across GC and compaction.
/// @code rbxx::object pinned{rbxx::value{ruby_value}}; @endcode
class object {
public:
  /// @brief Constructs an empty handle representing nil.
  object() noexcept = default;

  /// @brief Pins the supplied Ruby value.
  /// @code rbxx::object pinned{rbxx::value{Qtrue}}; @endcode
  explicit object(value initial) : cell_(std::make_shared<detail::object_cell>(initial.raw())) {}

  /// @brief Pins the supplied raw Ruby VALUE.
  explicit object(VALUE initial) : object(value{initial}) {}

  object(const object&) noexcept = default;
  object(object&&) noexcept = default;
  object& operator=(const object&) noexcept = default;
  object& operator=(object&&) noexcept = default;
  ~object() = default;

  /// @brief Returns the pinned object as a non-owning value view.
  /// @code rbxx::value current = pinned.get(); @endcode
  [[nodiscard]] value get() const noexcept { return value{raw()}; }

  /// @brief Returns the current raw VALUE, updated after compaction.
  [[nodiscard]] VALUE raw() const noexcept { return cell_ ? cell_->raw() : Qnil; }

  /// @brief Returns whether the handle is empty or pins nil.
  [[nodiscard]] bool is_nil() const noexcept { return NIL_P(raw()); }

  /// @brief Releases this handle's ownership of the pin.
  /// @code pinned.reset(); @endcode
  void reset() noexcept { cell_.reset(); }

private:
  std::shared_ptr<detail::object_cell> cell_;
};

} // namespace rbxx
