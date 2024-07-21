BigInt inverse_mod(const BigInt& n, const BigInt& mod)
   {
   if(mod.is_zero())
      throw BigInt::DivideByZero();
   if(mod.is_negative() || n.is_negative())
      throw Invalid_Argument("inverse_mod: arguments must be non-negative");

   if(n.is_zero() || (n.is_even() && mod.is_even()))
      return 0;

   BigInt x = mod, y = n, u = mod, v = n;
   BigInt A = 1, B = 0, C = 0, D = 1;

   while(u.is_nonzero())
      {
      size_t zero_bits = low_zero_bits(u);
      u >>= zero_bits;
      for(size_t i = 0; i != zero_bits; ++i)
         {
         if(A.is_odd() || B.is_odd())
            { A += y; B -= x; }
         A >>= 1; B >>= 1;
         }

      zero_bits = low_zero_bits(v);
      v >>= zero_bits;
      for(size_t i = 0; i != zero_bits; ++i)
         {
         if(C.is_odd() || D.is_odd())
            { C += y; D -= x; }
         C >>= 1; D >>= 1;
         }

      if(u >= v) { u -= v; A -= C; B -= D; }
      else       { v -= u; C -= A; D -= B; }
      }

   if(v != 1)
      return 0;

   while(D.is_negative()) D += mod;
   while(D >= mod) D -= mod;

   return D;
   }