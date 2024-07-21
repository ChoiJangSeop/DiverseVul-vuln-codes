static int uv__idna_toascii_label(const char* s, const char* se,
                                  char** d, char* de) {
  static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz0123456789";
  const char* ss;
  unsigned c;
  unsigned h;
  unsigned k;
  unsigned n;
  unsigned m;
  unsigned q;
  unsigned t;
  unsigned x;
  unsigned y;
  unsigned bias;
  unsigned delta;
  unsigned todo;
  int first;

  h = 0;
  ss = s;
  todo = 0;

  foreach_codepoint(c, &s, se) {
    if (c < 128)
      h++;
    else if (c == (unsigned) -1)
      return UV_EINVAL;
    else
      todo++;
  }

  if (todo > 0) {
    if (*d < de) *(*d)++ = 'x';
    if (*d < de) *(*d)++ = 'n';
    if (*d < de) *(*d)++ = '-';
    if (*d < de) *(*d)++ = '-';
  }

  x = 0;
  s = ss;
  foreach_codepoint(c, &s, se) {
    if (c > 127)
      continue;

    if (*d < de)
      *(*d)++ = c;

    if (++x == h)
      break;  /* Visited all ASCII characters. */
  }

  if (todo == 0)
    return h;

  /* Only write separator when we've written ASCII characters first. */
  if (h > 0)
    if (*d < de)
      *(*d)++ = '-';

  n = 128;
  bias = 72;
  delta = 0;
  first = 1;

  while (todo > 0) {
    m = -1;
    s = ss;
    foreach_codepoint(c, &s, se)
      if (c >= n)
        if (c < m)
          m = c;

    x = m - n;
    y = h + 1;

    if (x > ~delta / y)
      return UV_E2BIG;  /* Overflow. */

    delta += x * y;
    n = m;

    s = ss;
    foreach_codepoint(c, &s, se) {
      if (c < n)
        if (++delta == 0)
          return UV_E2BIG;  /* Overflow. */

      if (c != n)
        continue;

      for (k = 36, q = delta; /* empty */; k += 36) {
        t = 1;

        if (k > bias)
          t = k - bias;

        if (t > 26)
          t = 26;

        if (q < t)
          break;

        /* TODO(bnoordhuis) Since 1 <= t <= 26 and therefore
         * 10 <= y <= 35, we can optimize the long division
         * into a table-based reciprocal multiplication.
         */
        x = q - t;
        y = 36 - t;  /* 10 <= y <= 35 since 1 <= t <= 26. */
        q = x / y;
        t = t + x % y;  /* 1 <= t <= 35 because of y. */

        if (*d < de)
          *(*d)++ = alphabet[t];
      }

      if (*d < de)
        *(*d)++ = alphabet[q];

      delta /= 2;

      if (first) {
        delta /= 350;
        first = 0;
      }

      /* No overflow check is needed because |delta| was just
       * divided by 2 and |delta+delta >= delta + delta/h|.
       */
      h++;
      delta += delta / h;

      for (bias = 0; delta > 35 * 26 / 2; bias += 36)
        delta /= 35;

      bias += 36 * delta / (delta + 38);
      delta = 0;
      todo--;
    }

    delta++;
    n++;
  }

  return 0;
}