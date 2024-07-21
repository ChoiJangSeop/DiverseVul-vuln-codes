memxor3_different_alignment_all (word_t *dst,
				 const char *a, const char *b,
				 unsigned a_offset, unsigned b_offset,
				 size_t n)
{
  int al, ar, bl, br;
  const word_t *a_word;
  const word_t *b_word;

  word_t a0, a1, b0, b1;

  al = CHAR_BIT * a_offset;
  ar = CHAR_BIT * (sizeof(word_t) - a_offset);
  bl = CHAR_BIT * b_offset;
  br = CHAR_BIT * (sizeof(word_t) - b_offset);

  a_word = (const word_t *) ((uintptr_t) a & -sizeof(word_t));
  b_word = (const word_t *) ((uintptr_t) b & -sizeof(word_t));

  if (n & 1)
    {
      n--;
      a1 = a_word[n]; a0 = a_word[n+1];
      b1 = b_word[n]; b0 = b_word[n+1];

      dst[n] = MERGE (a1, al, a0, ar) ^ MERGE (b1, bl, b0, br);
    }
  else
    {
      a1 = a_word[n];
      b1 = b_word[n];
    }

  while (n > 0)
    {
      n -= 2;
      a0 = a_word[n+1]; b0 = b_word[n+1];
      dst[n+1] = MERGE(a0, al, a1, ar) ^ MERGE(b0, bl, b1, br);
      a1 = a_word[n]; b1 = b_word[n];
      dst[n] = MERGE(a1, al, a0, ar) ^ MERGE(b1, bl, b0, br);
    }
}