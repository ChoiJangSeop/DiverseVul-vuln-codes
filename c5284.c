secret (gcry_mpi_t output, gcry_mpi_t input, RSA_secret_key *skey )
{
  /* Remove superfluous leading zeroes from INPUT.  */
  mpi_normalize (input);

  if (!skey->p || !skey->q || !skey->u)
    {
      mpi_powm (output, input, skey->d, skey->n);
    }
  else
    {
      gcry_mpi_t m1 = mpi_alloc_secure( mpi_get_nlimbs(skey->n)+1 );
      gcry_mpi_t m2 = mpi_alloc_secure( mpi_get_nlimbs(skey->n)+1 );
      gcry_mpi_t h  = mpi_alloc_secure( mpi_get_nlimbs(skey->n)+1 );

      /* m1 = c ^ (d mod (p-1)) mod p */
      mpi_sub_ui( h, skey->p, 1  );
      mpi_fdiv_r( h, skey->d, h );
      mpi_powm( m1, input, h, skey->p );
      /* m2 = c ^ (d mod (q-1)) mod q */
      mpi_sub_ui( h, skey->q, 1  );
      mpi_fdiv_r( h, skey->d, h );
      mpi_powm( m2, input, h, skey->q );
      /* h = u * ( m2 - m1 ) mod q */
      mpi_sub( h, m2, m1 );
      if ( mpi_has_sign ( h ) )
        mpi_add ( h, h, skey->q );
      mpi_mulm( h, skey->u, h, skey->q );
      /* m = m1 + h * p */
      mpi_mul ( h, h, skey->p );
      mpi_add ( output, m1, h );

      mpi_free ( h );
      mpi_free ( m1 );
      mpi_free ( m2 );
    }
}