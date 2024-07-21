curve448_eh_to_x (mp_limb_t *xp, const mp_limb_t *p, mp_limb_t *scratch)
{
#define up p
#define vp (p + ecc->p.size)
#define t0 scratch
#define tp (scratch + ecc->p.size)

  const struct ecc_curve *ecc = &_nettle_curve448;
  mp_limb_t cy;

  /* If u = U/W and v = V/W are the coordinates of the point on
     edwards448 we get the curve448 x coordinate as

     x = v^2 / u^2 = (V/W)^2 / (U/W)^2 = (V/U)^2
  */
  /* Needs a total of 5*size storage. */
  ecc->p.invert (&ecc->p, t0, up, tp);
  ecc_mod_mul (&ecc->p, t0, t0, vp, tp);
  ecc_mod_sqr (&ecc->p, t0, t0, tp);

  cy = mpn_sub_n (xp, t0, ecc->p.m, ecc->p.size);
  cnd_copy (cy, xp, t0, ecc->p.size);
#undef up
#undef vp
#undef t0
#undef tp
}