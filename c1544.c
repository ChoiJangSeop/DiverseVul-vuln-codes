static int rsa_prepare_blinding( rsa_context *ctx,
                 int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    int ret;

    if( ctx->Vf.p != NULL )
    {
        /* We already have blinding values, just update them by squaring */
        MPI_CHK( mpi_mul_mpi( &ctx->Vi, &ctx->Vi, &ctx->Vi ) );
        MPI_CHK( mpi_mod_mpi( &ctx->Vi, &ctx->Vi, &ctx->N ) );
        MPI_CHK( mpi_mul_mpi( &ctx->Vf, &ctx->Vf, &ctx->Vf ) );
        MPI_CHK( mpi_mod_mpi( &ctx->Vf, &ctx->Vf, &ctx->N ) );

        return( 0 );
    }

    /* Unblinding value: Vf = random number */
    MPI_CHK( mpi_fill_random( &ctx->Vf, ctx->len - 1, f_rng, p_rng ) );

    /* Mathematically speaking, the algorithm should check Vf
     * against 0, P and Q (Vf should be relatively prime to N, and 0 < Vf < N),
     * so that Vf^-1 exists.
     */

    /* Blinding value: Vi =  Vf^(-e) mod N */
    MPI_CHK( mpi_inv_mod( &ctx->Vi, &ctx->Vf, &ctx->N ) );
    MPI_CHK( mpi_exp_mod( &ctx->Vi, &ctx->Vi, &ctx->E, &ctx->N, &ctx->RN ) );

cleanup:
    return( ret );
}