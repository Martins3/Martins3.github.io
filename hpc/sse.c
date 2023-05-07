// code from here :
// https://stackoverflow.com/questions/1389712/getting-started-with-intel-x86-sse-simd-instructions
#include <immintrin.h> // portable to all x86 compilers
#include <stdio.h>

int main() {
  __m128 vector1 = _mm_set_ps(
      4.0, 3.0, 2.0,
      1.0); // high element first, opposite of C array order.  Use _mm_setr_ps
            // if you want "little endian" element order in the source.
  __m128 vector2 = _mm_set_ps(7.0, 8.0, 9.0, 0.0);

  __m128 sum = _mm_add_ps(vector1, vector2); // result = vector1 + vector 2

  vector1 = _mm_shuffle_ps(vector1, vector1, _MM_SHUFFLE(0, 1, 2, 3));
  // vector1 is now (1, 2, 3, 4) (above shuffle reversed it)
  // TODO 靠，怎么输出啊
  return 0;
}
