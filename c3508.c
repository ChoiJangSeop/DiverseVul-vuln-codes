ecc_256_modq (const struct ecc_modulo *q, mp_limb_t *rp)
{
  mp_limb_t u2, u1, u0;
  mp_size_t n;

  n = 2*q->size;
  u2 = rp[--n];
  u1 = rp[n-1];

  /* This is not particularly fast, but should work well with assembly implementation. */
  for (; n >= q->size; n--)
    {
      mp_limb_t q2, q1, q0, t, c1, c0;

      u0 = rp[n-2];
      
      /* <q2, q1, q0> = v * u2 + <u2,u1>, same method as above.

	   +---+---+
	   | u2| u1|
	   +---+---+
	       |-u2|
	     +-+-+-+
	     | u2|
       +---+-+-+-+-+
       | q2| q1| q0|
       +---+---+---+
      */
      q1 = u2 - (u2 > u1);
      q0 = u1 - u2;
      t = u2 << 32;
      q0 += t;
      t = (u2 >> 32) + (q0 < t) + 1;
      q1 += t;
      q2 = q1 < t;

      /* Compute candidate remainder, <u1, u0> - <q2, q1> * (2^128 - 2^96 + 2^64 - 1)
         <u1, u0> + 2^64 q2 + (2^96 - 2^64 + 1) q1 (mod 2^128)

	   +---+---+
	   | u1| u0|
	   +---+---+
	   | q2| q1|
	   +---+---+
	   |-q1|
	 +-+-+-+
	 | q1|
       --+-+-+-+---+
           | u2| u1|
	   +---+---+
      */	 
      u2 = u1 + q2 - q1;
      u1 = u0 + q1;
      u2 += (u1 < q1);
      u2 += (q1 << 32);

      t = -(mp_limb_t) (u2 >= q0);
      q1 += t;
      q2 += t + (q1 < t);
      u1 += t;
      u2 += (t << 32) + (u1 < t);

      assert (q2 < 2);

      c0 = cnd_sub_n (q2, rp + n - 3, q->m, 1);
      c0 += (-q2) & q->m[1];
      t = mpn_submul_1 (rp + n - 4, q->m, 2, q1);
      c0 += t;
      c1 = c0 < t;
      
      /* Construct underflow condition. */
      c1 += (u1 < c0);
      t = - (mp_limb_t) (u2 < c1);

      u1 -= c0;
      u2 -= c1;

      /* Conditional add of p */
      u1 += t;
      u2 += (t<<32) + (u0 < t);

      t = cnd_add_n (t, rp + n - 4, q->m, 2);
      u1 += t;
      u2 += (u1 < t);
    }
  rp[2] = u1;
  rp[3] = u2;
}