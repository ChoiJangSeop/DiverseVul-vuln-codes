X509::X509(const char* i, size_t iSz, const char* s, size_t sSz,
           const char* b, int bSz, const char* a, int aSz)
    : issuer_(i, iSz), subject_(s, sSz),
      beforeDate_(b, bSz), afterDate_(a, aSz)
{}