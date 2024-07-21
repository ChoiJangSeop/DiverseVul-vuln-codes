mark_usable_uid_certs (kbnode_t keyblock, kbnode_t uidnode,
                       u32 *main_kid, struct key_item *klist,
                       u32 curtime, u32 *next_expire)
{
  kbnode_t node;
  PKT_signature *sig;

  /* First check all signatures.  */
  for (node=uidnode->next; node; node = node->next)
    {
      int rc;

      node->flag &= ~(1<<8 | 1<<9 | 1<<10 | 1<<11 | 1<<12);
      if (node->pkt->pkttype == PKT_USER_ID
          || node->pkt->pkttype == PKT_PUBLIC_SUBKEY)
        break; /* ready */
      if (node->pkt->pkttype != PKT_SIGNATURE)
        continue;
      sig = node->pkt->pkt.signature;
      if (main_kid
	  && sig->keyid[0] == main_kid[0] && sig->keyid[1] == main_kid[1])
        continue; /* ignore self-signatures if we pass in a main_kid */
      if (!IS_UID_SIG(sig) && !IS_UID_REV(sig))
        continue; /* we only look at these signature classes */
      if(sig->sig_class>=0x11 && sig->sig_class<=0x13 &&
	 sig->sig_class-0x10<opt.min_cert_level)
	continue; /* treat anything under our min_cert_level as an
		     invalid signature */
      if (klist && !is_in_klist (klist, sig))
        continue;  /* no need to check it then */
      if ((rc=check_key_signature (keyblock, node, NULL)))
	{
	  /* we ignore anything that won't verify, but tag the
	     no_pubkey case */
	  if (gpg_err_code (rc) == GPG_ERR_NO_PUBKEY)
            node->flag |= 1<<12;
          continue;
        }
      node->flag |= 1<<9;
    }
  /* Reset the remaining flags. */
  for (; node; node = node->next)
    node->flag &= ~(1<<8 | 1<<9 | 1<<10 | 1<<11 | 1<<12);

  /* kbnode flag usage: bit 9 is here set for signatures to consider,
   * bit 10 will be set by the loop to keep track of keyIDs already
   * processed, bit 8 will be set for the usable signatures, and bit
   * 11 will be set for usable revocations. */

  /* For each cert figure out the latest valid one.  */
  for (node=uidnode->next; node; node = node->next)
    {
      KBNODE n, signode;
      u32 kid[2];
      u32 sigdate;

      if (node->pkt->pkttype == PKT_PUBLIC_SUBKEY)
        break;
      if ( !(node->flag & (1<<9)) )
        continue; /* not a node to look at */
      if ( (node->flag & (1<<10)) )
        continue; /* signature with a keyID already processed */
      node->flag |= (1<<10); /* mark this node as processed */
      sig = node->pkt->pkt.signature;
      signode = node;
      sigdate = sig->timestamp;
      kid[0] = sig->keyid[0]; kid[1] = sig->keyid[1];

      /* Now find the latest and greatest signature */
      for (n=uidnode->next; n; n = n->next)
        {
          if (n->pkt->pkttype == PKT_PUBLIC_SUBKEY)
            break;
          if ( !(n->flag & (1<<9)) )
            continue;
          if ( (n->flag & (1<<10)) )
            continue; /* shortcut already processed signatures */
          sig = n->pkt->pkt.signature;
          if (kid[0] != sig->keyid[0] || kid[1] != sig->keyid[1])
            continue;
          n->flag |= (1<<10); /* mark this node as processed */

	  /* If signode is nonrevocable and unexpired and n isn't,
             then take signode (skip).  It doesn't matter which is
             older: if signode was older then we don't want to take n
             as signode is nonrevocable.  If n was older then we're
             automatically fine. */

	  if(((IS_UID_SIG(signode->pkt->pkt.signature) &&
	       !signode->pkt->pkt.signature->flags.revocable &&
	       (signode->pkt->pkt.signature->expiredate==0 ||
		signode->pkt->pkt.signature->expiredate>curtime))) &&
	     (!(IS_UID_SIG(n->pkt->pkt.signature) &&
		!n->pkt->pkt.signature->flags.revocable &&
		(n->pkt->pkt.signature->expiredate==0 ||
		 n->pkt->pkt.signature->expiredate>curtime))))
	    continue;

	  /* If n is nonrevocable and unexpired and signode isn't,
             then take n.  Again, it doesn't matter which is older: if
             n was older then we don't want to take signode as n is
             nonrevocable.  If signode was older then we're
             automatically fine. */

	  if((!(IS_UID_SIG(signode->pkt->pkt.signature) &&
		!signode->pkt->pkt.signature->flags.revocable &&
		(signode->pkt->pkt.signature->expiredate==0 ||
		 signode->pkt->pkt.signature->expiredate>curtime))) &&
	     ((IS_UID_SIG(n->pkt->pkt.signature) &&
	       !n->pkt->pkt.signature->flags.revocable &&
	       (n->pkt->pkt.signature->expiredate==0 ||
		n->pkt->pkt.signature->expiredate>curtime))))
            {
              signode = n;
              sigdate = sig->timestamp;
	      continue;
            }

	  /* At this point, if it's newer, it goes in as the only
             remaining possibilities are signode and n are both either
             revocable or expired or both nonrevocable and unexpired.
             If the timestamps are equal take the later ordered
             packet, presuming that the key packets are hopefully in
             their original order. */

          if (sig->timestamp >= sigdate)
            {
              signode = n;
              sigdate = sig->timestamp;
            }
        }

      sig = signode->pkt->pkt.signature;
      if (IS_UID_SIG (sig))
        { /* this seems to be a usable one which is not revoked.
           * Just need to check whether there is an expiration time,
           * We do the expired certification after finding a suitable
           * certification, the assumption is that a signator does not
           * want that after the expiration of his certificate the
           * system falls back to an older certification which has a
           * different expiration time */
          const byte *p;
          u32 expire;

          p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_SIG_EXPIRE, NULL );
          expire = p? sig->timestamp + buffer_to_u32(p) : 0;

          if (expire==0 || expire > curtime )
            {
              signode->flag |= (1<<8); /* yeah, found a good cert */
              if (next_expire && expire && expire < *next_expire)
                *next_expire = expire;
            }
        }
      else
	signode->flag |= (1<<11);
    }
}