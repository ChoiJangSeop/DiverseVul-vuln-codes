parse_key_usage (PKT_signature * sig)
{
  int key_usage = 0;
  const byte *p;
  size_t n;
  byte flags;

  p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_KEY_FLAGS, &n);
  if (p && n)
    {
      /* First octet of the keyflags.  */
      flags = *p;

      if (flags & 1)
	{
	  key_usage |= PUBKEY_USAGE_CERT;
	  flags &= ~1;
	}

      if (flags & 2)
	{
	  key_usage |= PUBKEY_USAGE_SIG;
	  flags &= ~2;
	}

      /* We do not distinguish between encrypting communications and
         encrypting storage. */
      if (flags & (0x04 | 0x08))
	{
	  key_usage |= PUBKEY_USAGE_ENC;
	  flags &= ~(0x04 | 0x08);
	}

      if (flags & 0x20)
	{
	  key_usage |= PUBKEY_USAGE_AUTH;
	  flags &= ~0x20;
	}

      if (flags)
	key_usage |= PUBKEY_USAGE_UNKNOWN;
    }

  /* We set PUBKEY_USAGE_UNKNOWN to indicate that this key has a
     capability that we do not handle.  This serves to distinguish
     between a zero key usage which we handle as the default
     capabilities for that algorithm, and a usage that we do not
     handle. */

  return key_usage;
}