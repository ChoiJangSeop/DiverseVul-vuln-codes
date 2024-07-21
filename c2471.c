gpgsm_get_short_fingerprint (ksba_cert_t cert, unsigned long *r_high)
{
  unsigned char digest[20];

  gpgsm_get_fingerprint (cert, GCRY_MD_SHA1, digest, NULL);
  if (r_high)
    *r_high = (((unsigned long)digest[12]<<24)
               |(digest[13]<<16)
               |(digest[14]<< 8)
               |digest[15]);
  return (((unsigned long)digest[16]<<24)
          |(digest[17]<<16)
          |(digest[18]<<8)
          |digest[19]);
}