curve25519_eh_to_x (mp_limb_t *xp, const mp_limb_t *p,
		    mp_limb_t *scratch)
{
#define vp (p + ecc->p.size)
#define wp (p + 2*ecc->p.size)
#define t0 scratch
#define t1 (scratch + ecc->p.size)
#define tp (scratch + 2*ecc->p.size)

  const struct ecc_curve *ecc = &_nettle_curve25519;
  mp_limb_t cy;

  /* If u = U/W and v = V/W are the coordinates of the point on the
     Edwards curve we get the curve25519 x coordinate as

     x = (1+v) / (1-v) = (W + V) / (W - V)
  */
  /* NOTE: For the infinity point, this subtraction gives zero (mod
     p), which isn't invertible. For curve25519, the desired output is
     x = 0, and we should be fine, since ecc_mod_inv for ecc->p returns 0
     in this case. */
  ecc_mod_sub (&ecc->p, t0, wp, vp);
  /* Needs a total of 6*size storage. */
  ecc->p.invert (&ecc->p, t1, t0, tp);
  
  ecc_mod_add (&ecc->p, t0, wp, vp);
  ecc_mod_mul (&ecc->p, t0, t0, t1, tp);

  cy = mpn_sub_n (xp, t0, ecc->p.m, ecc->p.size);
  cnd_copy (cy, xp, t0, ecc->p.size);
#undef vp
#undef wp
#undef t0
#undef t1
#undef tp
}