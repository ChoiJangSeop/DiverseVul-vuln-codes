static int32_t gcd(int a, int b)
{
  if (a == 0 && b == 0) {
    return 1;
  }

  if (a == 0) return b;
  if (b == 0) return a;

  int32_t h;

  do {
    h = a % b;
    a = b;
    b = h;
  } while (b != 0);

  return a;
}