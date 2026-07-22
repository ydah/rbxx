#define RBXX_DEBUG 1
#include <rbxx/rbxx.hpp>

#include <atomic>
#include <chrono>
#include <exception>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace rx = rbxx;

namespace {

struct child {
  explicit child(int initial) : number(initial) {}
  [[nodiscard]] int value() const { return number; }
  int number;
};

struct owner {
  static inline std::atomic<int> destroyed_count = 0;

  explicit owner(int initial) : owned_child(initial) {}
  ~owner() { ++destroyed_count; }
  [[nodiscard]] child* get_child() { return &owned_child; }

  child owned_child;
};

int apply_callback(const std::function<int(int)>& callback, int input) { return callback(input); }

struct callback_payload {
  const std::function<int(int)>* callback;
  std::exception_ptr exception;
};

void* call_callback_worker(void* opaque) noexcept {
  auto* payload = static_cast<callback_payload*>(opaque);
  try {
    (*payload->callback)(1);
  } catch (...) {
    payload->exception = std::current_exception();
  }
  return nullptr;
}

int apply_callback_without_gvl(const std::function<int(int)>& callback) {
  callback_payload payload{&callback, nullptr};
  rx::protect([&payload] {
    rb_thread_call_without_gvl(call_callback_worker, &payload, RUBY_UBF_IO, nullptr);
  });
  if (payload.exception) {
    std::rethrow_exception(payload.exception);
  }
  return 0;
}

int blocking_work(int milliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
  return milliseconds;
}

std::atomic<bool> stop_requested = false;

int interruptible_work(int milliseconds) {
  stop_requested.store(false, std::memory_order_release);
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
  while (!stop_requested.load(std::memory_order_acquire) &&
         std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return stop_requested.load(std::memory_order_acquire) ? 1 : 0;
}

void request_stop() noexcept { stop_requested.store(true, std::memory_order_release); }

struct int_buffer {
  explicit int_buffer(std::vector<int> input) : values(std::move(input)) {}

  auto begin() { return values.begin(); }
  auto end() { return values.end(); }

  std::vector<int> values;
};

} // namespace

RBXX_EXTENSION(advanced) {
  rx::module root = rx::define_module("RbxxTest");
  VALUE nested =
      rx::protect([root] { return rb_define_module_under(root.get().raw(), "Advanced"); });
  rx::module advanced{rx::value{nested}};

  advanced.def_class<child>("Child").def(rx::init<int>()).def("value", &child::value);
  advanced.def_class<owner>("Owner")
      .def(rx::init<int>())
      .def("child", &owner::get_child, rx::policy::reference, rx::keep_alive<0, 1>())
      .def("unsafe_child", &owner::get_child, rx::policy::reference)
      .def_static("destroyed", [] { return owner::destroyed_count.load(); });

  advanced.def("apply_callback", &apply_callback)
      .def("apply_callback_without_gvl", &apply_callback_without_gvl)
      .def("blocking_work", rx::nogvl(&blocking_work))
      .def("interruptible_work", rx::nogvl_interruptible(&interruptible_work, &request_stop));

  advanced.def_class<int_buffer>("IntBuffer")
      .def(rx::init<std::vector<int>>())
      .def_iterable<&int_buffer::begin, &int_buffer::end>();
}
