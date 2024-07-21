ECDSA_PrivateKey::create_signature_op(RandomNumberGenerator& /*rng*/,
                                      const std::string& params,
                                      const std::string& provider) const
   {
#if defined(BOTAN_HAS_BEARSSL)
   if(provider == "bearssl" || provider.empty())
      {
      try
         {
         return make_bearssl_ecdsa_sig_op(*this, params);
         }
      catch(Lookup_Error& e)
         {
         if(provider == "bearssl")
            throw;
         }
      }
#endif

#if defined(BOTAN_HAS_OPENSSL)
   if(provider == "openssl" || provider.empty())
      {
      try
         {
         return make_openssl_ecdsa_sig_op(*this, params);
         }
      catch(Lookup_Error& e)
         {
         if(provider == "openssl")
            throw;
         }
      }
#endif

   if(provider == "base" || provider.empty())
      return std::unique_ptr<PK_Ops::Signature>(new ECDSA_Signature_Operation(*this, params));

   throw Provider_Not_Found(algo_name(), provider);
   }