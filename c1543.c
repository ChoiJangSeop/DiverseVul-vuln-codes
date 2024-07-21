int rsa_private( rsa_context *ctx,
                 int (*f_rng)(void *, unsigned char *, size_t),
                 void *p_rng,
                 const unsigned char *input,
                 unsigned char *output )
{
    int ret;
    size_t olen;
    mpi T, T1, T2;

    mpi_init( &T ); mpi_init( &T1 ); mpi_init( &T2 );

    MPI_CHK( mpi_read_binary( &T, input, ctx->len ) );

    if( mpi_cmp_mpi( &T, &ctx->N ) >= 0 )
    {
        mpi_free( &T );
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
    }

#if defined(POLARSSL_RSA_NO_CRT)
    ((void) f_rng);
    ((void) p_rng);
    MPI_CHK( mpi_exp_mod( &T, &T, &ctx->D, &ctx->N, &ctx->RN ) );
#else
    if( f_rng != NULL )
    {
        /*
         * Blinding
         * T = T * Vi mod N
         */
        MPI_CHK( rsa_prepare_blinding( ctx, f_rng, p_rng ) );
        MPI_CHK( mpi_mul_mpi( &T, &T, &ctx->Vi ) );
        MPI_CHK( mpi_mod_mpi( &T, &T, &ctx->N ) );
    }

    /*
     * faster decryption using the CRT
     *
     * T1 = input ^ dP mod P
     * T2 = input ^ dQ mod Q
     */
    MPI_CHK( mpi_exp_mod( &T1, &T, &ctx->DP, &ctx->P, &ctx->RP ) );
    MPI_CHK( mpi_exp_mod( &T2, &T, &ctx->DQ, &ctx->Q, &ctx->RQ ) );

    /*
     * T = (T1 - T2) * (Q^-1 mod P) mod P
     */
    MPI_CHK( mpi_sub_mpi( &T, &T1, &T2 ) );
    MPI_CHK( mpi_mul_mpi( &T1, &T, &ctx->QP ) );
    MPI_CHK( mpi_mod_mpi( &T, &T1, &ctx->P ) );

    /*
     * output = T2 + T * Q
     */
    MPI_CHK( mpi_mul_mpi( &T1, &T, &ctx->Q ) );
    MPI_CHK( mpi_add_mpi( &T, &T2, &T1 ) );

    if( f_rng != NULL )
    {
        /*
         * Unblind
         * T = T * Vf mod N
         */
        MPI_CHK( mpi_mul_mpi( &T, &T, &ctx->Vf ) );
        MPI_CHK( mpi_mod_mpi( &T, &T, &ctx->N ) );
    }
#endif

    olen = ctx->len;
    MPI_CHK( mpi_write_binary( &T, output, olen ) );

cleanup:

    mpi_free( &T ); mpi_free( &T1 ); mpi_free( &T2 );

    if( ret != 0 )
        return( POLARSSL_ERR_RSA_PRIVATE_FAILED + ret );

    return( 0 );
}