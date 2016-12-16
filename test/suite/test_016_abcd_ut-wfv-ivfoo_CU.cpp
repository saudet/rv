typedef float float4 __attribute__((ext_vector_type(4)));

#include <cmath>

extern "C"
void
foo(int i, float * A) {
  float v = A[i];
A: // br :U B C
  if (A[0] > 0) goto B; else goto C;
B: // br :T C D
  v = A[(2*i % 4)];
  if (A[i] > 0.0) goto C; else goto D;
C: // br D
  v = fabs(A[(i + 1) % 4]);
D:
  A[i] = v * v;
}