keyid_from_fingerprint( const byte *fprint, size_t fprint_len, u32 *keyid )
{
  u32 dummy_keyid[2];

  if( !keyid )
    keyid = dummy_keyid;

  if (fprint_len != 20)
    {
      /* This is special as we have to lookup the key first.  */
      PKT_public_key pk;
      int rc;

      memset (&pk, 0, sizeof pk);
      rc = get_pubkey_byfprint (&pk, fprint, fprint_len);
      if( rc )
        {
          log_error("Oops: keyid_from_fingerprint: no pubkey\n");
          keyid[0] = 0;
          keyid[1] = 0;
        }
      else
        keyid_from_pk (&pk, keyid);
    }
  else
    {
      const byte *dp = fprint;
      keyid[0] = dp[12] << 24 | dp[13] << 16 | dp[14] << 8 | dp[15] ;
      keyid[1] = dp[16] << 24 | dp[17] << 16 | dp[18] << 8 | dp[19] ;
    }

  return keyid[1];
}