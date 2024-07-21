_gcry_ecc_gost_sign (gcry_mpi_t input, ECC_secret_key *skey,
                     gcry_mpi_t r, gcry_mpi_t s)
{
  gpg_err_code_t rc = 0;
  gcry_mpi_t k, dr, sum, ke, x, e;
  mpi_point_struct I;
  gcry_mpi_t hash;
  const void *abuf;
  unsigned int abits, qbits;
  mpi_ec_t ctx;

  if (DBG_CIPHER)
    log_mpidump ("gost sign hash  ", input );

  qbits = mpi_get_nbits (skey->E.n);

  /* Convert the INPUT into an MPI if needed.  */
  if (mpi_is_opaque (input))
    {
      abuf = mpi_get_opaque (input, &abits);
      rc = _gcry_mpi_scan (&hash, GCRYMPI_FMT_USG, abuf, (abits+7)/8, NULL);
      if (rc)
        return rc;
      if (abits > qbits)
        mpi_rshift (hash, hash, abits - qbits);
    }
  else
    hash = input;


  k = NULL;
  dr = mpi_alloc (0);
  sum = mpi_alloc (0);
  ke = mpi_alloc (0);
  e = mpi_alloc (0);
  x = mpi_alloc (0);
  point_init (&I);

  ctx = _gcry_mpi_ec_p_internal_new (skey->E.model, skey->E.dialect, 0,
                                     skey->E.p, skey->E.a, skey->E.b);

  mpi_mod (e, input, skey->E.n); /* e = hash mod n */

  if (!mpi_cmp_ui (e, 0))
    mpi_set_ui (e, 1);

  /* Two loops to avoid R or S are zero.  This is more of a joke than
     a real demand because the probability of them being zero is less
     than any hardware failure.  Some specs however require it.  */
  do
    {
      do
        {
          mpi_free (k);
          k = _gcry_dsa_gen_k (skey->E.n, GCRY_STRONG_RANDOM);

          _gcry_mpi_ec_mul_point (&I, k, &skey->E.G, ctx);
          if (_gcry_mpi_ec_get_affine (x, NULL, &I, ctx))
            {
              if (DBG_CIPHER)
                log_debug ("ecc sign: Failed to get affine coordinates\n");
              rc = GPG_ERR_BAD_SIGNATURE;
              goto leave;
            }
          mpi_mod (r, x, skey->E.n);  /* r = x mod n */
        }
      while (!mpi_cmp_ui (r, 0));
      mpi_mulm (dr, skey->d, r, skey->E.n); /* dr = d*r mod n  */
      mpi_mulm (ke, k, e, skey->E.n); /* ke = k*e mod n */
      mpi_addm (s, ke, dr, skey->E.n); /* sum = (k*e+ d*r) mod n  */
    }
  while (!mpi_cmp_ui (s, 0));

  if (DBG_CIPHER)
    {
      log_mpidump ("gost sign result r ", r);
      log_mpidump ("gost sign result s ", s);
    }

 leave:
  _gcry_mpi_ec_free (ctx);
  point_free (&I);
  mpi_free (x);
  mpi_free (e);
  mpi_free (ke);
  mpi_free (sum);
  mpi_free (dr);
  mpi_free (k);

  if (hash != input)
    mpi_free (hash);

  return rc;
}