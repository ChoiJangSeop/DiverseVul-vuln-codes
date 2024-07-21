ec_p_init (mpi_ec_t ctx, enum gcry_mpi_ec_models model,
           enum ecc_dialects dialect,
           int flags,
           gcry_mpi_t p, gcry_mpi_t a, gcry_mpi_t b)
{
  int i;
  static int use_barrett;

  if (!use_barrett)
    {
      if (getenv ("GCRYPT_BARRETT"))
        use_barrett = 1;
      else
        use_barrett = -1;
    }

  /* Fixme: Do we want to check some constraints? e.g.  a < p  */

  ctx->model = model;
  ctx->dialect = dialect;
  ctx->flags = flags;
  if (dialect == ECC_DIALECT_ED25519)
    ctx->nbits = 256;
  else
    ctx->nbits = mpi_get_nbits (p);
  ctx->p = mpi_copy (p);
  ctx->a = mpi_copy (a);
  ctx->b = mpi_copy (b);

  ctx->t.p_barrett = use_barrett > 0? _gcry_mpi_barrett_init (ctx->p, 0):NULL;

  _gcry_mpi_ec_get_reset (ctx);

  /* Allocate scratch variables.  */
  for (i=0; i< DIM(ctx->t.scratch); i++)
    ctx->t.scratch[i] = mpi_alloc_like (ctx->p);

  /* Prepare for fast reduction.  */
  /* FIXME: need a test for NIST values.  However it does not gain us
     any real advantage, for 384 bits it is actually slower than using
     mpi_mulm.  */
/*   ctx->nist_nbits = mpi_get_nbits (ctx->p); */
/*   if (ctx->nist_nbits == 192) */
/*     { */
/*       for (i=0; i < 4; i++) */
/*         ctx->s[i] = mpi_new (192); */
/*       ctx->c    = mpi_new (192*2); */
/*     } */
/*   else if (ctx->nist_nbits == 384) */
/*     { */
/*       for (i=0; i < 10; i++) */
/*         ctx->s[i] = mpi_new (384); */
/*       ctx->c    = mpi_new (384*2); */
/*     } */
}