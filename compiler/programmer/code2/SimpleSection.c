int printf(const char *format, ...);

int global_init_var = 84;
int global_uninit_var;

extern int fuck;

void func1(int i) { printf("%d\n", i); }
__attribute__((section("FOO"))) int global = 42;

int main(void) {
  static int static_var = 85;
  static int static_var2;
  int a = 1;
  int b;

  func1(static_var + static_var2 + a + b);

  return a;
}
