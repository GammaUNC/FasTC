#include <smmintrin.h>

int main() {
  const __m128 fv = _mm_set1_ps(1.0f);
  const __m128 fv2 = _mm_set1_ps(2.0f);

  const __m128 ans = _mm_blend_ps(fv, fv2, 2);

  return ((int *)(&ans))[0];
}
