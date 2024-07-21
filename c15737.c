DSA_PrivateKey::create_signature_op(RandomNumberGenerator& /*rng*/,
                                    const std::string& params,
                                    const std::string& provider) const
   {
   if(provider == "base" || provider.empty())
      return std::unique_ptr<PK_Ops::Signature>(new DSA_Signature_Operation(*this, params));
   throw Provider_Not_Found(algo_name(), provider);
   }