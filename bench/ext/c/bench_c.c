#include <ruby.h>

typedef struct {
  int value;
} bench_counter;

static void counter_free(void* pointer) { xfree(pointer); }
static size_t counter_size(const void* pointer) {
  return pointer == NULL ? 0 : sizeof(bench_counter);
}

static const rb_data_type_t counter_type = {
    "BenchC::Counter",
    {NULL, counter_free, counter_size, NULL, {NULL}},
    NULL,
    NULL,
    RUBY_TYPED_FREE_IMMEDIATELY,
};

static VALUE counter_allocate(VALUE klass) {
  bench_counter* counter = NULL;
  return TypedData_Make_Struct(klass, bench_counter, &counter_type, counter);
}

static VALUE counter_initialize(VALUE self, VALUE initial) {
  bench_counter* counter = NULL;
  TypedData_Get_Struct(self, bench_counter, &counter_type, counter);
  counter->value = NUM2INT(initial);
  return Qnil;
}

static VALUE counter_value(VALUE self) {
  bench_counter* counter = NULL;
  TypedData_Get_Struct(self, bench_counter, &counter_type, counter);
  return INT2FIX(counter->value);
}

static VALUE int_value(VALUE self) {
  (void)self;
  return INT2FIX(42);
}

static VALUE string_roundtrip(VALUE self, VALUE input) {
  (void)self;
  StringValue(input);
  return rb_str_dup(input);
}

static VALUE vector_roundtrip(VALUE self, VALUE input) {
  (void)self;
  Check_Type(input, T_ARRAY);
  const long size = RARRAY_LEN(input);
  VALUE result = rb_ary_new_capa(size);
  for (long index = 0; index < size; ++index) {
    rb_ary_push(result, DBL2NUM(NUM2DBL(rb_ary_entry(input, index))));
  }
  return result;
}

void Init_bench_c(void) {
  VALUE module = rb_define_module("BenchC");
  rb_define_module_function(module, "int_value", int_value, 0);
  rb_define_module_function(module, "string_roundtrip", string_roundtrip, 1);
  rb_define_module_function(module, "vector_roundtrip", vector_roundtrip, 1);

  VALUE klass = rb_define_class_under(module, "Counter", rb_cObject);
  rb_define_alloc_func(klass, counter_allocate);
  rb_define_method(klass, "initialize", counter_initialize, 1);
  rb_define_method(klass, "value", counter_value, 0);
}
