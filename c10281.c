static void TransformSignature(SignatureInfo *signature_info)
{
#define Ch(x,y,z)  (((x) & (y)) ^ (~(x) & (z)))
#define Maj(x,y,z)  (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define RotateRight(x,n)  (Trunc32(((x) >> n) | ((x) << (32-n))))
#define Sigma0(x)  (RotateRight(x,7) ^ RotateRight(x,18) ^ Trunc32((x) >> 3))
#define Sigma1(x)  (RotateRight(x,17) ^ RotateRight(x,19) ^ Trunc32((x) >> 10))
#define Suma0(x)  (RotateRight(x,2) ^ RotateRight(x,13) ^ RotateRight(x,22))
#define Suma1(x)  (RotateRight(x,6) ^ RotateRight(x,11) ^ RotateRight(x,25))
#define Trunc32(x)  ((unsigned int) ((x) & 0xffffffffU))

  ssize_t
    i;

  unsigned char
    *p;

  ssize_t
    j;

  static const unsigned int
    K[64] =
    {
      0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU,
      0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U, 0xd807aa98U, 0x12835b01U,
      0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U,
      0xc19bf174U, 0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
      0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU, 0x983e5152U,
      0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U,
      0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU,
      0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
      0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U,
      0xd6990624U, 0xf40e3585U, 0x106aa070U, 0x19a4c116U, 0x1e376c08U,
      0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU,
      0x682e6ff3U, 0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
      0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
    };  /* 32-bit fractional part of the cube root of the first 64 primes */

  unsigned int
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    shift,
    T,
    T1,
    T2,
    W[64];

  shift=32;
  p=GetStringInfoDatum(signature_info->message);
  if (signature_info->lsb_first == MagickFalse)
    {
DisableMSCWarning(4127)
      if (sizeof(unsigned int) <= 4)
RestoreMSCWarning
        for (i=0; i < 16; i++)
        {
          T=(*((unsigned int *) p));
          p+=4;
          W[i]=Trunc32(T);
        }
      else
        for (i=0; i < 16; i+=2)
        {
          T=(*((unsigned int *) p));
          p+=8;
          W[i]=Trunc32(T >> shift);
          W[i+1]=Trunc32(T);
        }
    }
  else
DisableMSCWarning(4127)
    if (sizeof(unsigned int) <= 4)
RestoreMSCWarning
      for (i=0; i < 16; i++)
      {
        T=(*((unsigned int *) p));
        p+=4;
        W[i]=((T << 24) & 0xff000000) | ((T << 8) & 0x00ff0000) |
          ((T >> 8) & 0x0000ff00) | ((T >> 24) & 0x000000ff);
      }
    else
      for (i=0; i < 16; i+=2)
      {
        T=(*((unsigned int *) p));
        p+=8;
        W[i]=((T << 24) & 0xff000000) | ((T << 8) & 0x00ff0000) |
          ((T >> 8) & 0x0000ff00) | ((T >> 24) & 0x000000ff);
        T>>=shift;
        W[i+1]=((T << 24) & 0xff000000) | ((T << 8) & 0x00ff0000) |
          ((T >> 8) & 0x0000ff00) | ((T >> 24) & 0x000000ff);
      }
  /*
    Copy accumulator to registers.
  */
  A=signature_info->accumulator[0];
  B=signature_info->accumulator[1];
  C=signature_info->accumulator[2];
  D=signature_info->accumulator[3];
  E=signature_info->accumulator[4];
  F=signature_info->accumulator[5];
  G=signature_info->accumulator[6];
  H=signature_info->accumulator[7];
  for (i=16; i < 64; i++)
    W[i]=Trunc32(Sigma1(W[i-2])+W[i-7]+Sigma0(W[i-15])+W[i-16]);
  for (j=0; j < 64; j++)
  {
    T1=Trunc32(H+Suma1(E)+Ch(E,F,G)+K[j]+W[j]);
    T2=Trunc32(Suma0(A)+Maj(A,B,C));
    H=G;
    G=F;
    F=E;
    E=Trunc32(D+T1);
    D=C;
    C=B;
    B=A;
    A=Trunc32(T1+T2);
  }
  /*
    Add registers back to accumulator.
  */
  signature_info->accumulator[0]=Trunc32(signature_info->accumulator[0]+A);
  signature_info->accumulator[1]=Trunc32(signature_info->accumulator[1]+B);
  signature_info->accumulator[2]=Trunc32(signature_info->accumulator[2]+C);
  signature_info->accumulator[3]=Trunc32(signature_info->accumulator[3]+D);
  signature_info->accumulator[4]=Trunc32(signature_info->accumulator[4]+E);
  signature_info->accumulator[5]=Trunc32(signature_info->accumulator[5]+F);
  signature_info->accumulator[6]=Trunc32(signature_info->accumulator[6]+G);
  signature_info->accumulator[7]=Trunc32(signature_info->accumulator[7]+H);
  /*
    Reset working registers.
  */
  A=0;
  B=0;
  C=0;
  D=0;
  E=0;
  F=0;
  G=0;
  H=0;
  T=0;
  T1=0;
  T2=0;
  (void) memset(W,0,sizeof(W));
}