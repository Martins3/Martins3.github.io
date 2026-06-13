#include "attr.h"
#include <stdio.h>

static void simple_foo() { printf("%s \n", __FUNCTION__); }

weak_alias(simple_foo, complex_foo);

void foo() { complex_foo(); }
