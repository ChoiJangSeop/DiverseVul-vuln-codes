fixup_uidnode (KBNODE uidnode, KBNODE signode, u32 keycreated)
{
  PKT_user_id *uid = uidnode->pkt->pkt.user_id;
  PKT_signature *sig = signode->pkt->pkt.signature;
  const byte *p, *sym, *hash, *zip;
  size_t n, nsym, nhash, nzip;

  sig->flags.chosen_selfsig = 1;/* We chose this one. */
  uid->created = 0;		/* Not created == invalid. */
  if (IS_UID_REV (sig))
    {
      uid->is_revoked = 1;
      return; /* Has been revoked.  */
    }
  else
    uid->is_revoked = 0;

  uid->expiredate = sig->expiredate;

  if (sig->flags.expired)
    {
      uid->is_expired = 1;
      return; /* Has expired.  */
    }
  else
    uid->is_expired = 0;

  uid->created = sig->timestamp; /* This one is okay. */
  uid->selfsigversion = sig->version;
  /* If we got this far, it's not expired :) */
  uid->is_expired = 0;

  /* Store the key flags in the helper variable for later processing.  */
  uid->help_key_usage = parse_key_usage (sig);

  /* Ditto for the key expiration.  */
  p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_KEY_EXPIRE, NULL);
  if (p && buffer_to_u32 (p))
    uid->help_key_expire = keycreated + buffer_to_u32 (p);
  else
    uid->help_key_expire = 0;

  /* Set the primary user ID flag - we will later wipe out some
   * of them to only have one in our keyblock.  */
  uid->is_primary = 0;
  p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_PRIMARY_UID, NULL);
  if (p && *p)
    uid->is_primary = 2;

  /* We could also query this from the unhashed area if it is not in
   * the hased area and then later try to decide which is the better
   * there should be no security problem with this.
   * For now we only look at the hashed one.  */

  /* Now build the preferences list.  These must come from the
     hashed section so nobody can modify the ciphers a key is
     willing to accept.  */
  p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_PREF_SYM, &n);
  sym = p;
  nsym = p ? n : 0;
  p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_PREF_HASH, &n);
  hash = p;
  nhash = p ? n : 0;
  p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_PREF_COMPR, &n);
  zip = p;
  nzip = p ? n : 0;
  if (uid->prefs)
    xfree (uid->prefs);
  n = nsym + nhash + nzip;
  if (!n)
    uid->prefs = NULL;
  else
    {
      uid->prefs = xmalloc (sizeof (*uid->prefs) * (n + 1));
      n = 0;
      for (; nsym; nsym--, n++)
	{
	  uid->prefs[n].type = PREFTYPE_SYM;
	  uid->prefs[n].value = *sym++;
	}
      for (; nhash; nhash--, n++)
	{
	  uid->prefs[n].type = PREFTYPE_HASH;
	  uid->prefs[n].value = *hash++;
	}
      for (; nzip; nzip--, n++)
	{
	  uid->prefs[n].type = PREFTYPE_ZIP;
	  uid->prefs[n].value = *zip++;
	}
      uid->prefs[n].type = PREFTYPE_NONE; /* End of list marker  */
      uid->prefs[n].value = 0;
    }

  /* See whether we have the MDC feature.  */
  uid->flags.mdc = 0;
  p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_FEATURES, &n);
  if (p && n && (p[0] & 0x01))
    uid->flags.mdc = 1;

  /* And the keyserver modify flag.  */
  uid->flags.ks_modify = 1;
  p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_KS_FLAGS, &n);
  if (p && n && (p[0] & 0x80))
    uid->flags.ks_modify = 0;
}