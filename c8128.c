sign (gcry_mpi_t r, gcry_mpi_t s, gcry_mpi_t input, DSA_secret_key *skey,
      int flags, int hashalgo)
{
  gpg_err_code_t rc;
  gcry_mpi_t hash;
  gcry_mpi_t k;
  gcry_mpi_t kinv;
  gcry_mpi_t tmp;
  const void *abuf;
  unsigned int abits, qbits;
  int extraloops = 0;

  qbits = mpi_get_nbits (skey->q);

  /* Convert the INPUT into an MPI.  */
  rc = _gcry_dsa_normalize_hash (input, &hash, qbits);
  if (rc)
    return rc;

 again:
  /* Create the K value.  */
  if ((flags & PUBKEY_FLAG_RFC6979) && hashalgo)
    {
      /* Use Pornin's method for deterministic DSA.  If this flag is
         set, it is expected that HASH is an opaque MPI with the to be
         signed hash.  That hash is also used as h1 from 3.2.a.  */
      if (!mpi_is_opaque (input))
        {
          rc = GPG_ERR_CONFLICT;
          goto leave;
        }

      abuf = mpi_get_opaque (input, &abits);
      rc = _gcry_dsa_gen_rfc6979_k (&k, skey->q, skey->x,
                                    abuf, (abits+7)/8, hashalgo, extraloops);
      if (rc)
        goto leave;
    }
  else
    {
      /* Select a random k with 0 < k < q */
      k = _gcry_dsa_gen_k (skey->q, GCRY_STRONG_RANDOM);
    }

  /* r = (a^k mod p) mod q */
  mpi_powm( r, skey->g, k, skey->p );
  mpi_fdiv_r( r, r, skey->q );

  /* kinv = k^(-1) mod q */
  kinv = mpi_alloc( mpi_get_nlimbs(k) );
  mpi_invm(kinv, k, skey->q );

  /* s = (kinv * ( hash + x * r)) mod q */
  tmp = mpi_alloc( mpi_get_nlimbs(skey->p) );
  mpi_mul( tmp, skey->x, r );
  mpi_add( tmp, tmp, hash );
  mpi_mulm( s , kinv, tmp, skey->q );

  mpi_free(k);
  mpi_free(kinv);
  mpi_free(tmp);

  if (!mpi_cmp_ui (r, 0))
    {
      /* This is a highly unlikely code path.  */
      extraloops++;
      goto again;
    }

  rc = 0;

 leave:
  if (hash != input)
    mpi_free (hash);

  return rc;
}