// insights default.cpp -- -std=c++20
struct Plain {
  int value = 0;
};

struct CopyAssign {
  int value = 0;
  CopyAssign &operator=(const CopyAssign &) = default;
};

struct MoveAssign {
  int value = 0;
  MoveAssign &operator=(MoveAssign &&) = default;
};

struct Restored {
  int value = 0;
  Restored() = default;
  Restored(const Restored &) = default;
  Restored(Restored &&) = default;
  Restored &operator=(const Restored &) = default;
  Restored &operator=(Restored &&) = default;
  ~Restored() = default;
};

int main() {
  Plain a;
  Plain b(a);
  Plain c(static_cast<Plain &&>(a));
  b = a;
  c = static_cast<Plain &&>(a);

  CopyAssign d;
  CopyAssign e(d);
  CopyAssign f(static_cast<CopyAssign &&>(d)); // Calls copy constructor.
  e = d;
  f = static_cast<CopyAssign &&>(d); // Calls copy assignment.

  MoveAssign g;
  MoveAssign h;
  h = static_cast<MoveAssign &&>(g);
  // MoveAssign i(g); // Error: implicitly deleted copy constructor.
  // MoveAssign j(static_cast<MoveAssign &&>(g)); // Same error: no move ctor.
  // h = g; // Error: implicitly deleted copy assignment.

  Restored k;
  Restored l(k);
  Restored m(static_cast<Restored &&>(k));
  l = k;
  m = static_cast<Restored &&>(k);
  return b.value + c.value + e.value + f.value + h.value + l.value + m.value;
}
