parse_signature (IOBUF inp, int pkttype, unsigned long pktlen,
		 PKT_signature * sig)
{
  int md5_len = 0;
  unsigned n;
  int is_v4 = 0;
  int rc = 0;
  int i, ndata;

  if (pktlen < 16)
    {
      log_error ("packet(%d) too short\n", pkttype);
      if (list_mode)
        es_fputs (":signature packet: [too short]\n", listfp);
      goto leave;
    }
  sig->version = iobuf_get_noeof (inp);
  pktlen--;
  if (sig->version == 4)
    is_v4 = 1;
  else if (sig->version != 2 && sig->version != 3)
    {
      log_error ("packet(%d) with unknown version %d\n",
		 pkttype, sig->version);
      if (list_mode)
        es_fputs (":signature packet: [unknown version]\n", listfp);
      rc = gpg_error (GPG_ERR_INV_PACKET);
      goto leave;
    }

  if (!is_v4)
    {
      md5_len = iobuf_get_noeof (inp);
      pktlen--;
    }
  sig->sig_class = iobuf_get_noeof (inp);
  pktlen--;
  if (!is_v4)
    {
      sig->timestamp = read_32 (inp);
      pktlen -= 4;
      sig->keyid[0] = read_32 (inp);
      pktlen -= 4;
      sig->keyid[1] = read_32 (inp);
      pktlen -= 4;
    }
  sig->pubkey_algo = iobuf_get_noeof (inp);
  pktlen--;
  sig->digest_algo = iobuf_get_noeof (inp);
  pktlen--;
  sig->flags.exportable = 1;
  sig->flags.revocable = 1;
  if (is_v4) /* Read subpackets.  */
    {
      n = read_16 (inp);
      pktlen -= 2;  /* Length of hashed data. */
      if (n > 10000)
	{
	  log_error ("signature packet: hashed data too long\n");
          if (list_mode)
            es_fputs (":signature packet: [hashed data too long]\n", listfp);
	  rc = GPG_ERR_INV_PACKET;
	  goto leave;
	}
      if (n)
	{
	  sig->hashed = xmalloc (sizeof (*sig->hashed) + n - 1);
	  sig->hashed->size = n;
	  sig->hashed->len = n;
	  if (iobuf_read (inp, sig->hashed->data, n) != n)
	    {
	      log_error ("premature eof while reading "
			 "hashed signature data\n");
              if (list_mode)
                es_fputs (":signature packet: [premature eof]\n", listfp);
	      rc = -1;
	      goto leave;
	    }
	  pktlen -= n;
	}
      n = read_16 (inp);
      pktlen -= 2;  /* Length of unhashed data.  */
      if (n > 10000)
	{
	  log_error ("signature packet: unhashed data too long\n");
          if (list_mode)
            es_fputs (":signature packet: [unhashed data too long]\n", listfp);
	  rc = GPG_ERR_INV_PACKET;
	  goto leave;
	}
      if (n)
	{
	  sig->unhashed = xmalloc (sizeof (*sig->unhashed) + n - 1);
	  sig->unhashed->size = n;
	  sig->unhashed->len = n;
	  if (iobuf_read (inp, sig->unhashed->data, n) != n)
	    {
	      log_error ("premature eof while reading "
			 "unhashed signature data\n");
              if (list_mode)
                es_fputs (":signature packet: [premature eof]\n", listfp);
	      rc = -1;
	      goto leave;
	    }
	  pktlen -= n;
	}
    }

  if (pktlen < 5)  /* Sanity check.  */
    {
      log_error ("packet(%d) too short\n", pkttype);
      if (list_mode)
        es_fputs (":signature packet: [too short]\n", listfp);
      rc = GPG_ERR_INV_PACKET;
      goto leave;
    }

  sig->digest_start[0] = iobuf_get_noeof (inp);
  pktlen--;
  sig->digest_start[1] = iobuf_get_noeof (inp);
  pktlen--;

  if (is_v4 && sig->pubkey_algo)  /* Extract required information.  */
    {
      const byte *p;
      size_t len;

      /* Set sig->flags.unknown_critical if there is a critical bit
       * set for packets which we do not understand.  */
      if (!parse_sig_subpkt (sig->hashed, SIGSUBPKT_TEST_CRITICAL, NULL)
	  || !parse_sig_subpkt (sig->unhashed, SIGSUBPKT_TEST_CRITICAL, NULL))
	sig->flags.unknown_critical = 1;

      p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_SIG_CREATED, NULL);
      if (p)
	sig->timestamp = buffer_to_u32 (p);
      else if (!(sig->pubkey_algo >= 100 && sig->pubkey_algo <= 110)
	       && opt.verbose)
	log_info ("signature packet without timestamp\n");

      p = parse_sig_subpkt2 (sig, SIGSUBPKT_ISSUER, NULL);
      if (p)
	{
	  sig->keyid[0] = buffer_to_u32 (p);
	  sig->keyid[1] = buffer_to_u32 (p + 4);
	}
      else if (!(sig->pubkey_algo >= 100 && sig->pubkey_algo <= 110)
	       && opt.verbose)
	log_info ("signature packet without keyid\n");

      p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_SIG_EXPIRE, NULL);
      if (p && buffer_to_u32 (p))
	sig->expiredate = sig->timestamp + buffer_to_u32 (p);
      if (sig->expiredate && sig->expiredate <= make_timestamp ())
	sig->flags.expired = 1;

      p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_POLICY, NULL);
      if (p)
	sig->flags.policy_url = 1;

      p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_PREF_KS, NULL);
      if (p)
	sig->flags.pref_ks = 1;

      p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_NOTATION, NULL);
      if (p)
	sig->flags.notation = 1;

      p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_REVOCABLE, NULL);
      if (p && *p == 0)
	sig->flags.revocable = 0;

      p = parse_sig_subpkt (sig->hashed, SIGSUBPKT_TRUST, &len);
      if (p && len == 2)
	{
	  sig->trust_depth = p[0];
	  sig->trust_value = p[1];

	  /* Only look for a regexp if there is also a trust
	     subpacket. */
	  sig->trust_regexp =
	    parse_sig_subpkt (sig->hashed, SIGSUBPKT_REGEXP, &len);

	  /* If the regular expression is of 0 length, there is no
	     regular expression. */
	  if (len == 0)
	    sig->trust_regexp = NULL;
	}

      /* We accept the exportable subpacket from either the hashed or
         unhashed areas as older versions of gpg put it in the
         unhashed area.  In theory, anyway, we should never see this
         packet off of a local keyring. */

      p = parse_sig_subpkt2 (sig, SIGSUBPKT_EXPORTABLE, NULL);
      if (p && *p == 0)
	sig->flags.exportable = 0;

      /* Find all revocation keys.  */
      if (sig->sig_class == 0x1F)
	parse_revkeys (sig);
    }

  if (list_mode)
    {
      es_fprintf (listfp, ":signature packet: algo %d, keyid %08lX%08lX\n"
                  "\tversion %d, created %lu, md5len %d, sigclass 0x%02x\n"
                  "\tdigest algo %d, begin of digest %02x %02x\n",
                  sig->pubkey_algo,
                  (ulong) sig->keyid[0], (ulong) sig->keyid[1],
                  sig->version, (ulong) sig->timestamp, md5_len, sig->sig_class,
                  sig->digest_algo, sig->digest_start[0], sig->digest_start[1]);
      if (is_v4)
	{
	  parse_sig_subpkt (sig->hashed, SIGSUBPKT_LIST_HASHED, NULL);
	  parse_sig_subpkt (sig->unhashed, SIGSUBPKT_LIST_UNHASHED, NULL);
	}
    }

  ndata = pubkey_get_nsig (sig->pubkey_algo);
  if (!ndata)
    {
      if (list_mode)
	es_fprintf (listfp, "\tunknown algorithm %d\n", sig->pubkey_algo);
      unknown_pubkey_warning (sig->pubkey_algo);

      /* We store the plain material in data[0], so that we are able
       * to write it back with build_packet().  */
      if (pktlen > (5 * MAX_EXTERN_MPI_BITS / 8))
	{
	  /* We include a limit to avoid too trivial DoS attacks by
	     having gpg allocate too much memory.  */
	  log_error ("signature packet: too much data\n");
	  rc = GPG_ERR_INV_PACKET;
	}
      else
	{
	  sig->data[0] =
	    gcry_mpi_set_opaque (NULL, read_rest (inp, pktlen), pktlen * 8);
	  pktlen = 0;
	}
    }
  else
    {
      for (i = 0; i < ndata; i++)
	{
	  n = pktlen;
	  sig->data[i] = mpi_read (inp, &n, 0);
	  pktlen -= n;
	  if (list_mode)
	    {
	      es_fprintf (listfp, "\tdata: ");
	      mpi_print (listfp, sig->data[i], mpi_print_mode);
	      es_putc ('\n', listfp);
	    }
	  if (!sig->data[i])
	    rc = GPG_ERR_INV_PACKET;
	}
    }

 leave:
  iobuf_skip_rest (inp, pktlen, 0);
  return rc;
}