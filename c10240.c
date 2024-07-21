ecc_j_to_a (const struct ecc_curve *ecc,
	    int op,
	    mp_limb_t *r, const mp_limb_t *p,
	    mp_limb_t *scratch)
{
#define izp   scratch
#define iz2p (scratch + ecc->p.size)
#define iz3p (scratch + 2*ecc->p.size)
#define tp    scratch

  mp_limb_t cy;

  ecc->p.invert (&ecc->p, izp, p+2*ecc->p.size, izp + ecc->p.size);
  ecc_mod_sqr (&ecc->p, iz2p, izp, iz2p);

  if (ecc->use_redc)
    {
      /* Divide this common factor by B, instead of applying redc to
	 both x and y outputs. */
      mpn_zero (iz2p + ecc->p.size, ecc->p.size);
      ecc->p.reduce (&ecc->p, iz2p, iz2p);
    }

  /* r_x <-- x / z^2 */
  ecc_mod_mul (&ecc->p, iz3p, iz2p, p, iz3p);
  /* ecc_mod (and ecc_mod_mul) may return a value up to 2p - 1, so
     do a conditional subtraction. */
  cy = mpn_sub_n (r, iz3p, ecc->p.m, ecc->p.size);
  cnd_copy (cy, r, iz3p, ecc->p.size);

  if (op)
    {
      /* Skip y coordinate */
      if (op > 1)
	{
	  /* Also reduce the x coordinate mod ecc->q. It should
	     already be < 2*ecc->q, so one subtraction should
	     suffice. */
	  cy = mpn_sub_n (scratch, r, ecc->q.m, ecc->p.size);
	  cnd_copy (cy == 0, r, scratch, ecc->p.size);
	}
      return;
    }
  ecc_mod_mul (&ecc->p, iz3p, iz2p, izp, iz3p);
  ecc_mod_mul (&ecc->p, tp, iz3p, p + ecc->p.size, tp);
  /* And a similar subtraction. */
  cy = mpn_sub_n (r + ecc->p.size, tp, ecc->p.m, ecc->p.size);
  cnd_copy (cy, r + ecc->p.size, tp, ecc->p.size);

#undef izp
#undef iz2p
#undef iz3p
#undef tp
}