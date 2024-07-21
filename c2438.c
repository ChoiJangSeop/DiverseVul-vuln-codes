keyid_from_pk (PKT_public_key *pk, u32 *keyid)
{
  u32 lowbits;
  u32 dummy_keyid[2];

  if (!keyid)
    keyid = dummy_keyid;

  if( pk->keyid[0] || pk->keyid[1] )
    {
      keyid[0] = pk->keyid[0];
      keyid[1] = pk->keyid[1];
      lowbits = keyid[1];
    }
  else
    {
      const byte *dp;
      gcry_md_hd_t md;

      md = do_fingerprint_md(pk);
      if(md)
	{
	  dp = gcry_md_read ( md, 0 );
	  keyid[0] = dp[12] << 24 | dp[13] << 16 | dp[14] << 8 | dp[15] ;
	  keyid[1] = dp[16] << 24 | dp[17] << 16 | dp[18] << 8 | dp[19] ;
	  lowbits = keyid[1];
	  gcry_md_close (md);
	  pk->keyid[0] = keyid[0];
	  pk->keyid[1] = keyid[1];
	}
      else
	pk->keyid[0]=pk->keyid[1]=keyid[0]=keyid[1]=lowbits=0xFFFFFFFF;
    }

  return lowbits;
}