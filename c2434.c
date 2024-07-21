merge_selfsigs_main (KBNODE keyblock, int *r_revoked,
		     struct revoke_info *rinfo)
{
  PKT_public_key *pk = NULL;
  KBNODE k;
  u32 kid[2];
  u32 sigdate, uiddate, uiddate2;
  KBNODE signode, uidnode, uidnode2;
  u32 curtime = make_timestamp ();
  unsigned int key_usage = 0;
  u32 keytimestamp = 0;
  u32 key_expire = 0;
  int key_expire_seen = 0;
  byte sigversion = 0;

  *r_revoked = 0;
  memset (rinfo, 0, sizeof (*rinfo));

  if (keyblock->pkt->pkttype != PKT_PUBLIC_KEY)
    BUG ();
  pk = keyblock->pkt->pkt.public_key;
  keytimestamp = pk->timestamp;

  keyid_from_pk (pk, kid);
  pk->main_keyid[0] = kid[0];
  pk->main_keyid[1] = kid[1];

  if (pk->version < 4)
    {
      /* Before v4 the key packet itself contains the expiration date
       * and there was no way to change it, so we start with the one
       * from the key packet.  */
      key_expire = pk->max_expiredate;
      key_expire_seen = 1;
    }

  /* First pass: Find the latest direct key self-signature.  We assume
   * that the newest one overrides all others.  */

  /* In case this key was already merged. */
  xfree (pk->revkey);
  pk->revkey = NULL;
  pk->numrevkeys = 0;

  signode = NULL;
  sigdate = 0; /* Helper variable to find the latest signature.  */
  for (k = keyblock; k && k->pkt->pkttype != PKT_USER_ID; k = k->next)
    {
      if (k->pkt->pkttype == PKT_SIGNATURE)
	{
	  PKT_signature *sig = k->pkt->pkt.signature;
	  if (sig->keyid[0] == kid[0] && sig->keyid[1] == kid[1])
	    {
	      if (check_key_signature (keyblock, k, NULL))
		; /* Signature did not verify.  */
	      else if (IS_KEY_REV (sig))
		{
		  /* Key has been revoked - there is no way to
		   * override such a revocation, so we theoretically
		   * can stop now.  We should not cope with expiration
		   * times for revocations here because we have to
		   * assume that an attacker can generate all kinds of
		   * signatures.  However due to the fact that the key
		   * has been revoked it does not harm either and by
		   * continuing we gather some more info on that
		   * key.  */
		  *r_revoked = 1;
		  sig_to_revoke_info (sig, rinfo);
		}
	      else if (IS_KEY_SIG (sig))
		{
		  /* Add any revocation keys onto the pk.  This is
		     particularly interesting since we normally only
		     get data from the most recent 1F signature, but
		     you need multiple 1F sigs to properly handle
		     revocation keys (PGP does it this way, and a
		     revocation key could be sensitive and hence in a
		     different signature). */
		  if (sig->revkey)
		    {
		      int i;

		      pk->revkey =
			xrealloc (pk->revkey, sizeof (struct revocation_key) *
				  (pk->numrevkeys + sig->numrevkeys));

		      for (i = 0; i < sig->numrevkeys; i++)
			memcpy (&pk->revkey[pk->numrevkeys++],
				sig->revkey[i],
				sizeof (struct revocation_key));
		    }

		  if (sig->timestamp >= sigdate)
		    {
		      if (sig->flags.expired)
			; /* Signature has expired - ignore it.  */
		      else
			{
			  sigdate = sig->timestamp;
			  signode = k;
			  if (sig->version > sigversion)
			    sigversion = sig->version;

			}
		    }
		}
	    }
	}
    }

  /* Remove dupes from the revocation keys.  */

  if (pk->revkey)
    {
      int i, j, x, changed = 0;

      for (i = 0; i < pk->numrevkeys; i++)
	{
	  for (j = i + 1; j < pk->numrevkeys; j++)
	    {
	      if (memcmp (&pk->revkey[i], &pk->revkey[j],
			  sizeof (struct revocation_key)) == 0)
		{
		  /* remove j */

		  for (x = j; x < pk->numrevkeys - 1; x++)
		    pk->revkey[x] = pk->revkey[x + 1];

		  pk->numrevkeys--;
		  j--;
		  changed = 1;
		}
	    }
	}

      if (changed)
	pk->revkey = xrealloc (pk->revkey,
			       pk->numrevkeys *
			       sizeof (struct revocation_key));
    }

  if (signode)
    {
      /* Some information from a direct key signature take precedence
       * over the same information given in UID sigs.  */
      PKT_signature *sig = signode->pkt->pkt.signature;
      const byte *p;

      key_usage = parse_key_usage (sig);

      p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_KEY_EXPIRE, NULL);
      if (p && buffer_to_u32 (p))
	{
	  key_expire = keytimestamp + buffer_to_u32 (p);
	  key_expire_seen = 1;
	}

      /* Mark that key as valid: One direct key signature should
       * render a key as valid.  */
      pk->flags.valid = 1;
    }

  /* Pass 1.5: Look for key revocation signatures that were not made
     by the key (i.e. did a revocation key issue a revocation for
     us?).  Only bother to do this if there is a revocation key in the
     first place and we're not revoked already.  */

  if (!*r_revoked && pk->revkey)
    for (k = keyblock; k && k->pkt->pkttype != PKT_USER_ID; k = k->next)
      {
	if (k->pkt->pkttype == PKT_SIGNATURE)
	  {
	    PKT_signature *sig = k->pkt->pkt.signature;

	    if (IS_KEY_REV (sig) &&
		(sig->keyid[0] != kid[0] || sig->keyid[1] != kid[1]))
	      {
		int rc = check_revocation_keys (pk, sig);
		if (rc == 0)
		  {
		    *r_revoked = 2;
		    sig_to_revoke_info (sig, rinfo);
		    /* Don't continue checking since we can't be any
		       more revoked than this.  */
		    break;
		  }
		else if (gpg_err_code (rc) == GPG_ERR_NO_PUBKEY)
		  pk->flags.maybe_revoked = 1;

		/* A failure here means the sig did not verify, was
		   not issued by a revocation key, or a revocation
		   key loop was broken.  If a revocation key isn't
		   findable, however, the key might be revoked and
		   we don't know it.  */

		/* TODO: In the future handle subkey and cert
		   revocations?  PGP doesn't, but it's in 2440. */
	      }
	  }
      }

  /* Second pass: Look at the self-signature of all user IDs.  */
  signode = uidnode = NULL;
  sigdate = 0; /* Helper variable to find the latest signature in one UID. */
  for (k = keyblock; k && k->pkt->pkttype != PKT_PUBLIC_SUBKEY; k = k->next)
    {
      if (k->pkt->pkttype == PKT_USER_ID)
	{
	  if (uidnode && signode)
	    {
	      fixup_uidnode (uidnode, signode, keytimestamp);
	      pk->flags.valid = 1;
	    }
	  uidnode = k;
	  signode = NULL;
	  sigdate = 0;
	}
      else if (k->pkt->pkttype == PKT_SIGNATURE && uidnode)
	{
	  PKT_signature *sig = k->pkt->pkt.signature;
	  if (sig->keyid[0] == kid[0] && sig->keyid[1] == kid[1])
	    {
	      if (check_key_signature (keyblock, k, NULL))
		;		/* signature did not verify */
	      else if ((IS_UID_SIG (sig) || IS_UID_REV (sig))
		       && sig->timestamp >= sigdate)
		{
		  /* Note: we allow to invalidate cert revocations
		   * by a newer signature.  An attacker can't use this
		   * because a key should be revoced with a key revocation.
		   * The reason why we have to allow for that is that at
		   * one time an email address may become invalid but later
		   * the same email address may become valid again (hired,
		   * fired, hired again).  */

		  sigdate = sig->timestamp;
		  signode = k;
		  signode->pkt->pkt.signature->flags.chosen_selfsig = 0;
		  if (sig->version > sigversion)
		    sigversion = sig->version;
		}
	    }
	}
    }
  if (uidnode && signode)
    {
      fixup_uidnode (uidnode, signode, keytimestamp);
      pk->flags.valid = 1;
    }

  /* If the key isn't valid yet, and we have
     --allow-non-selfsigned-uid set, then force it valid. */
  if (!pk->flags.valid && opt.allow_non_selfsigned_uid)
    {
      if (opt.verbose)
	log_info (_("Invalid key %s made valid by"
		    " --allow-non-selfsigned-uid\n"), keystr_from_pk (pk));
      pk->flags.valid = 1;
    }

  /* The key STILL isn't valid, so try and find an ultimately
     trusted signature. */
  if (!pk->flags.valid)
    {
      uidnode = NULL;

      for (k = keyblock; k && k->pkt->pkttype != PKT_PUBLIC_SUBKEY;
	   k = k->next)
	{
	  if (k->pkt->pkttype == PKT_USER_ID)
	    uidnode = k;
	  else if (k->pkt->pkttype == PKT_SIGNATURE && uidnode)
	    {
	      PKT_signature *sig = k->pkt->pkt.signature;

	      if (sig->keyid[0] != kid[0] || sig->keyid[1] != kid[1])
		{
		  PKT_public_key *ultimate_pk;

		  ultimate_pk = xmalloc_clear (sizeof (*ultimate_pk));

		  /* We don't want to use the full get_pubkey to
		     avoid infinite recursion in certain cases.
		     There is no reason to check that an ultimately
		     trusted key is still valid - if it has been
		     revoked or the user should also renmove the
		     ultimate trust flag.  */
		  if (get_pubkey_fast (ultimate_pk, sig->keyid) == 0
		      && check_key_signature2 (keyblock, k, ultimate_pk,
					       NULL, NULL, NULL, NULL) == 0
		      && get_ownertrust (ultimate_pk) == TRUST_ULTIMATE)
		    {
		      free_public_key (ultimate_pk);
		      pk->flags.valid = 1;
		      break;
		    }

		  free_public_key (ultimate_pk);
		}
	    }
	}
    }

  /* Record the highest selfsig version so we know if this is a v3
     key through and through, or a v3 key with a v4 selfsig
     somewhere.  This is useful in a few places to know if the key
     must be treated as PGP2-style or OpenPGP-style.  Note that a
     selfsig revocation with a higher version number will also raise
     this value.  This is okay since such a revocation must be
     issued by the user (i.e. it cannot be issued by someone else to
     modify the key behavior.) */

  pk->selfsigversion = sigversion;

  /* Now that we had a look at all user IDs we can now get some information
   * from those user IDs.
   */

  if (!key_usage)
    {
      /* Find the latest user ID with key flags set. */
      uiddate = 0; /* Helper to find the latest user ID.  */
      for (k = keyblock; k && k->pkt->pkttype != PKT_PUBLIC_SUBKEY;
	   k = k->next)
	{
	  if (k->pkt->pkttype == PKT_USER_ID)
	    {
	      PKT_user_id *uid = k->pkt->pkt.user_id;
	      if (uid->help_key_usage && uid->created > uiddate)
		{
		  key_usage = uid->help_key_usage;
		  uiddate = uid->created;
		}
	    }
	}
    }
  if (!key_usage)
    {
      /* No key flags at all: get it from the algo.  */
      key_usage = openpgp_pk_algo_usage (pk->pubkey_algo);
    }
  else
    {
      /* Check that the usage matches the usage as given by the algo.  */
      int x = openpgp_pk_algo_usage (pk->pubkey_algo);
      if (x) /* Mask it down to the actual allowed usage.  */
	key_usage &= x;
    }

  /* Whatever happens, it's a primary key, so it can certify. */
  pk->pubkey_usage = key_usage | PUBKEY_USAGE_CERT;

  if (!key_expire_seen)
    {
      /* Find the latest valid user ID with a key expiration set
       * Note, that this may be a different one from the above because
       * some user IDs may have no expiration date set.  */
      uiddate = 0;
      for (k = keyblock; k && k->pkt->pkttype != PKT_PUBLIC_SUBKEY;
	   k = k->next)
	{
	  if (k->pkt->pkttype == PKT_USER_ID)
	    {
	      PKT_user_id *uid = k->pkt->pkt.user_id;
	      if (uid->help_key_expire && uid->created > uiddate)
		{
		  key_expire = uid->help_key_expire;
		  uiddate = uid->created;
		}
	    }
	}
    }

  /* Currently only v3 keys have a maximum expiration date, but I'll
     bet v5 keys get this feature again. */
  if (key_expire == 0
      || (pk->max_expiredate && key_expire > pk->max_expiredate))
    key_expire = pk->max_expiredate;

  pk->has_expired = key_expire >= curtime ? 0 : key_expire;
  pk->expiredate = key_expire;

  /* Fixme: we should see how to get rid of the expiretime fields  but
   * this needs changes at other places too. */

  /* And now find the real primary user ID and delete all others.  */
  uiddate = uiddate2 = 0;
  uidnode = uidnode2 = NULL;
  for (k = keyblock; k && k->pkt->pkttype != PKT_PUBLIC_SUBKEY; k = k->next)
    {
      if (k->pkt->pkttype == PKT_USER_ID && !k->pkt->pkt.user_id->attrib_data)
	{
	  PKT_user_id *uid = k->pkt->pkt.user_id;
	  if (uid->is_primary)
	    {
	      if (uid->created > uiddate)
		{
		  uiddate = uid->created;
		  uidnode = k;
		}
	      else if (uid->created == uiddate && uidnode)
		{
		  /* The dates are equal, so we need to do a
		     different (and arbitrary) comparison.  This
		     should rarely, if ever, happen.  It's good to
		     try and guarantee that two different GnuPG
		     users with two different keyrings at least pick
		     the same primary. */
		  if (cmp_user_ids (uid, uidnode->pkt->pkt.user_id) > 0)
		    uidnode = k;
		}
	    }
	  else
	    {
	      if (uid->created > uiddate2)
		{
		  uiddate2 = uid->created;
		  uidnode2 = k;
		}
	      else if (uid->created == uiddate2 && uidnode2)
		{
		  if (cmp_user_ids (uid, uidnode2->pkt->pkt.user_id) > 0)
		    uidnode2 = k;
		}
	    }
	}
    }
  if (uidnode)
    {
      for (k = keyblock; k && k->pkt->pkttype != PKT_PUBLIC_SUBKEY;
	   k = k->next)
	{
	  if (k->pkt->pkttype == PKT_USER_ID &&
	      !k->pkt->pkt.user_id->attrib_data)
	    {
	      PKT_user_id *uid = k->pkt->pkt.user_id;
	      if (k != uidnode)
		uid->is_primary = 0;
	    }
	}
    }
  else if (uidnode2)
    {
      /* None is flagged primary - use the latest user ID we have,
         and disambiguate with the arbitrary packet comparison. */
      uidnode2->pkt->pkt.user_id->is_primary = 1;
    }
  else
    {
      /* None of our uids were self-signed, so pick the one that
         sorts first to be the primary.  This is the best we can do
         here since there are no self sigs to date the uids. */

      uidnode = NULL;

      for (k = keyblock; k && k->pkt->pkttype != PKT_PUBLIC_SUBKEY;
	   k = k->next)
	{
	  if (k->pkt->pkttype == PKT_USER_ID
	      && !k->pkt->pkt.user_id->attrib_data)
	    {
	      if (!uidnode)
		{
		  uidnode = k;
		  uidnode->pkt->pkt.user_id->is_primary = 1;
		  continue;
		}
	      else
		{
		  if (cmp_user_ids (k->pkt->pkt.user_id,
				    uidnode->pkt->pkt.user_id) > 0)
		    {
		      uidnode->pkt->pkt.user_id->is_primary = 0;
		      uidnode = k;
		      uidnode->pkt->pkt.user_id->is_primary = 1;
		    }
		  else
		    k->pkt->pkt.user_id->is_primary = 0;	/* just to be
								   safe */
		}
	    }
	}
    }
}