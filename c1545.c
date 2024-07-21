void rsa_free( rsa_context *ctx )
{
#if !defined(POLARSSL_RSA_NO_CRT)
    mpi_free( &ctx->Vi ); mpi_free( &ctx->Vf );
#endif
    mpi_free( &ctx->RQ ); mpi_free( &ctx->RP ); mpi_free( &ctx->RN );
    mpi_free( &ctx->QP ); mpi_free( &ctx->DQ ); mpi_free( &ctx->DP );
    mpi_free( &ctx->Q  ); mpi_free( &ctx->P  ); mpi_free( &ctx->D );
    mpi_free( &ctx->E  ); mpi_free( &ctx->N  );
}