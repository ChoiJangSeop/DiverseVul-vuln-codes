merge_selfsigs_subkey (KBNODE keyblock, KBNODE subnode)
{
  PKT_public_key *mainpk = NULL, *subpk = NULL;
  PKT_signature *sig;
  KBNODE k;
  u32 mainkid[2];
  u32 sigdate = 0;
  KBNODE signode;
  u32 curtime = make_timestamp ();
  unsigned int key_usage = 0;
  u32 keytimestamp = 0;
  u32 key_expire = 0;
  const byte *p;

  if (subnode->pkt->pkttype != PKT_PUBLIC_SUBKEY)
    BUG ();
  mainpk = keyblock->pkt->pkt.public_key;
  if (mainpk->version < 4)
    return;/* (actually this should never happen) */
  keyid_from_pk (mainpk, mainkid);
  subpk = subnode->pkt->pkt.public_key;
  keytimestamp = subpk->timestamp;

  subpk->flags.valid = 0;
  subpk->main_keyid[0] = mainpk->main_keyid[0];
  subpk->main_keyid[1] = mainpk->main_keyid[1];

  /* Find the latest key binding self-signature.  */
  signode = NULL;
  sigdate = 0; /* Helper to find the latest signature.  */
  for (k = subnode->next; k && k->pkt->pkttype != PKT_PUBLIC_SUBKEY;
       k = k->next)
    {
      if (k->pkt->pkttype == PKT_SIGNATURE)
	{
	  sig = k->pkt->pkt.signature;
	  if (sig->keyid[0] == mainkid[0] && sig->keyid[1] == mainkid[1])
	    {
	      if (check_key_signature (keyblock, k, NULL))
		; /* Signature did not verify.  */
	      else if (IS_SUBKEY_REV (sig))
		{
		  /* Note that this means that the date on a
		     revocation sig does not matter - even if the
		     binding sig is dated after the revocation sig,
		     the subkey is still marked as revoked.  This
		     seems ok, as it is just as easy to make new
		     subkeys rather than re-sign old ones as the
		     problem is in the distribution.  Plus, PGP (7)
		     does this the same way.  */
		  subpk->flags.revoked = 1;
		  sig_to_revoke_info (sig, &subpk->revoked);
		  /* Although we could stop now, we continue to
		   * figure out other information like the old expiration
		   * time.  */
		}
	      else if (IS_SUBKEY_SIG (sig) && sig->timestamp >= sigdate)
		{
		  if (sig->flags.expired)
		    ; /* Signature has expired - ignore it.  */
		  else
		    {
		      sigdate = sig->timestamp;
		      signode = k;
		      signode->pkt->pkt.signature->flags.chosen_selfsig = 0;
		    }
		}
	    }
	}
    }

  /* No valid key binding.  */
  if (!signode)
    return;

  sig = signode->pkt->pkt.signature;
  sig->flags.chosen_selfsig = 1; /* So we know which selfsig we chose later.  */

  key_usage = parse_key_usage (sig);
  if (!key_usage)
    {
      /* No key flags at all: get it from the algo.  */
      key_usage = openpgp_pk_algo_usage (subpk->pubkey_algo);
    }
  else
    {
      /* Check that the usage matches the usage as given by the algo.  */
      int x = openpgp_pk_algo_usage (subpk->pubkey_algo);
      if (x) /* Mask it down to the actual allowed usage.  */
	key_usage &= x;
    }

  subpk->pubkey_usage = key_usage;

  p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_KEY_EXPIRE, NULL);
  if (p && buffer_to_u32 (p))
    key_expire = keytimestamp + buffer_to_u32 (p);
  else
    key_expire = 0;
  subpk->has_expired = key_expire >= curtime ? 0 : key_expire;
  subpk->expiredate = key_expire;

  /* Algo doesn't exist.  */
  if (openpgp_pk_test_algo (subpk->pubkey_algo))
    return;

  subpk->flags.valid = 1;

  /* Find the most recent 0x19 embedded signature on our self-sig. */
  if (!subpk->flags.backsig)
    {
      int seq = 0;
      size_t n;
      PKT_signature *backsig = NULL;

      sigdate = 0;

      /* We do this while() since there may be other embedded
         signatures in the future.  We only want 0x19 here. */

      while ((p = enum_sig_subpkt (sig->hashed,
				   SIGSUBPKT_SIGNATURE, &n, &seq, NULL)))
	if (n > 3
	    && ((p[0] == 3 && p[2] == 0x19) || (p[0] == 4 && p[1] == 0x19)))
	  {
	    PKT_signature *tempsig = buf_to_sig (p, n);
	    if (tempsig)
	      {
		if (tempsig->timestamp > sigdate)
		  {
		    if (backsig)
		      free_seckey_enc (backsig);

		    backsig = tempsig;
		    sigdate = backsig->timestamp;
		  }
		else
		  free_seckey_enc (tempsig);
	      }
	  }

      seq = 0;

      /* It is safe to have this in the unhashed area since the 0x19
         is located on the selfsig for convenience, not security. */

      while ((p = enum_sig_subpkt (sig->unhashed, SIGSUBPKT_SIGNATURE,
				   &n, &seq, NULL)))
	if (n > 3
	    && ((p[0] == 3 && p[2] == 0x19) || (p[0] == 4 && p[1] == 0x19)))
	  {
	    PKT_signature *tempsig = buf_to_sig (p, n);
	    if (tempsig)
	      {
		if (tempsig->timestamp > sigdate)
		  {
		    if (backsig)
		      free_seckey_enc (backsig);

		    backsig = tempsig;
		    sigdate = backsig->timestamp;
		  }
		else
		  free_seckey_enc (tempsig);
	      }
	  }

      if (backsig)
	{
	  /* At ths point, backsig contains the most recent 0x19 sig.
	     Let's see if it is good. */

	  /* 2==valid, 1==invalid, 0==didn't check */
	  if (check_backsig (mainpk, subpk, backsig) == 0)
	    subpk->flags.backsig = 2;
	  else
	    subpk->flags.backsig = 1;

	  free_seckey_enc (backsig);
	}
    }
}