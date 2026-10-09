#include "counter.hpp"

namespace {
int count = 0;
int increment() { return ++count; }
}

namespace tutorial {
int next() { return increment(); }
}
