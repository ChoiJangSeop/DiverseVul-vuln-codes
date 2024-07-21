SecureVector<byte> EME_PKCS1v15::unpad(const byte in[], size_t inlen,
                                       size_t key_len) const
   {
   if(inlen != key_len / 8 || inlen < 10 || in[0] != 0x02)
      throw Decoding_Error("PKCS1::unpad");

   size_t seperator = 0;
   for(size_t j = 0; j != inlen; ++j)
      if(in[j] == 0)
         {
         seperator = j;
         break;
         }
   if(seperator < 9)
      throw Decoding_Error("PKCS1::unpad");

   return SecureVector<byte>(in + seperator + 1, inlen - seperator - 1);
   }