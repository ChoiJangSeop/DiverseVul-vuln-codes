get_it (PKT_pubkey_enc *enc, DEK *dek, PKT_public_key *sk, u32 *keyid)
{
  gpg_error_t err;
  byte *frame = NULL;
  unsigned int n;
  size_t nframe;
  u16 csum, csum2;
  int padding;
  gcry_sexp_t s_data;
  char *desc;
  char *keygrip;
  byte fp[MAX_FINGERPRINT_LEN];
  size_t fpn;

  if (DBG_CLOCK)
    log_clock ("decryption start");

  /* Get the keygrip.  */
  err = hexkeygrip_from_pk (sk, &keygrip);
  if (err)
    goto leave;

  /* Convert the data to an S-expression.  */
  if (sk->pubkey_algo == PUBKEY_ALGO_ELGAMAL
      || sk->pubkey_algo == PUBKEY_ALGO_ELGAMAL_E)
    {
      if (!enc->data[0] || !enc->data[1])
        err = gpg_error (GPG_ERR_BAD_MPI);
      else
        err = gcry_sexp_build (&s_data, NULL, "(enc-val(elg(a%m)(b%m)))",
                               enc->data[0], enc->data[1]);
    }
  else if (sk->pubkey_algo == PUBKEY_ALGO_RSA
           || sk->pubkey_algo == PUBKEY_ALGO_RSA_E)
    {
      if (!enc->data[0])
        err = gpg_error (GPG_ERR_BAD_MPI);
      else
        err = gcry_sexp_build (&s_data, NULL, "(enc-val(rsa(a%m)))",
                               enc->data[0]);
    }
  else if (sk->pubkey_algo == PUBKEY_ALGO_ECDH)
    {
      if (!enc->data[0] || !enc->data[1])
        err = gpg_error (GPG_ERR_BAD_MPI);
      else
        err = gcry_sexp_build (&s_data, NULL, "(enc-val(ecdh(s%m)(e%m)))",
                               enc->data[1], enc->data[0]);
    }
  else
    err = gpg_error (GPG_ERR_BUG);

  if (err)
    goto leave;

  if (sk->pubkey_algo == PUBKEY_ALGO_ECDH)
    {
      fingerprint_from_pk (sk, fp, &fpn);
      assert (fpn == 20);
    }

  /* Decrypt. */
  desc = gpg_format_keydesc (sk, FORMAT_KEYDESC_NORMAL, 1);
  err = agent_pkdecrypt (NULL, keygrip,
                         desc, sk->keyid, sk->main_keyid, sk->pubkey_algo,
                         s_data, &frame, &nframe, &padding);
  xfree (desc);
  gcry_sexp_release (s_data);
  if (err)
    goto leave;

  /* Now get the DEK (data encryption key) from the frame
   *
   * Old versions encode the DEK in in this format (msb is left):
   *
   *     0  1  DEK(16 bytes)  CSUM(2 bytes)  0  RND(n bytes) 2
   *
   * Later versions encode the DEK like this:
   *
   *     0  2  RND(n bytes)  0  A  DEK(k bytes)  CSUM(2 bytes)
   *
   * (mpi_get_buffer already removed the leading zero).
   *
   * RND are non-zero randow bytes.
   * A   is the cipher algorithm
   * DEK is the encryption key (session key) with length k
   * CSUM
   */
  if (DBG_CIPHER)
    log_printhex ("DEK frame:", frame, nframe);
  n = 0;

  if (sk->pubkey_algo == PUBKEY_ALGO_ECDH)
    {
      gcry_mpi_t shared_mpi;
      gcry_mpi_t decoded;

      /* At the beginning the frame are the bytes of shared point MPI.  */
      err = gcry_mpi_scan (&shared_mpi, GCRYMPI_FMT_USG, frame, nframe, NULL);
      if (err)
        {
          err = gpg_error (GPG_ERR_WRONG_SECKEY);
          goto leave;
        }

      err = pk_ecdh_decrypt (&decoded, fp, enc->data[1]/*encr data as an MPI*/,
                             shared_mpi, sk->pkey);
      mpi_release (shared_mpi);
      if(err)
        goto leave;

      /* Reuse NFRAME, which size is sufficient to include the session key.  */
      err = gcry_mpi_print (GCRYMPI_FMT_USG, frame, nframe, &nframe, decoded);
      mpi_release (decoded);
      if (err)
        goto leave;

      /* Now the frame are the bytes decrypted but padded session key.  */

      /* Allow double padding for the benefit of DEK size concealment.
         Higher than this is wasteful. */
      if (!nframe || frame[nframe-1] > 8*2 || nframe <= 8
          || frame[nframe-1] > nframe)
        {
          err = gpg_error (GPG_ERR_WRONG_SECKEY);
          goto leave;
        }
      nframe -= frame[nframe-1]; /* Remove padding.  */
      assert (!n); /* (used just below) */
    }
  else
    {
      if (padding)
        {
          if (n + 7 > nframe)
            {
              err = gpg_error (GPG_ERR_WRONG_SECKEY);
              goto leave;
            }
          if (frame[n] == 1 && frame[nframe - 1] == 2)
            {
              log_info (_("old encoding of the DEK is not supported\n"));
              err = gpg_error (GPG_ERR_CIPHER_ALGO);
              goto leave;
            }
          if (frame[n] != 2) /* Something went wrong.  */
            {
              err = gpg_error (GPG_ERR_WRONG_SECKEY);
              goto leave;
            }
          for (n++; n < nframe && frame[n]; n++) /* Skip the random bytes.  */
            ;
          n++; /* Skip the zero byte.  */
        }
    }

  if (n + 4 > nframe)
    {
      err = gpg_error (GPG_ERR_WRONG_SECKEY);
      goto leave;
    }

  dek->keylen = nframe - (n + 1) - 2;
  dek->algo = frame[n++];
  err = openpgp_cipher_test_algo (dek->algo);
  if (err)
    {
      if (!opt.quiet && gpg_err_code (err) == GPG_ERR_CIPHER_ALGO)
        {
          log_info (_("cipher algorithm %d%s is unknown or disabled\n"),
                    dek->algo,
                    dek->algo == CIPHER_ALGO_IDEA ? " (IDEA)" : "");
        }
      dek->algo = 0;
      goto leave;
    }
  if (dek->keylen != openpgp_cipher_get_algo_keylen (dek->algo))
    {
      err = gpg_error (GPG_ERR_WRONG_SECKEY);
      goto leave;
    }

  /* Copy the key to DEK and compare the checksum.  */
  csum = frame[nframe - 2] << 8;
  csum |= frame[nframe - 1];
  memcpy (dek->key, frame + n, dek->keylen);
  for (csum2 = 0, n = 0; n < dek->keylen; n++)
    csum2 += dek->key[n];
  if (csum != csum2)
    {
      err = gpg_error (GPG_ERR_WRONG_SECKEY);
      goto leave;
    }
  if (DBG_CLOCK)
    log_clock ("decryption ready");
  if (DBG_CIPHER)
    log_printhex ("DEK is:", dek->key, dek->keylen);

  /* Check that the algo is in the preferences and whether it has expired.  */
  {
    PKT_public_key *pk = NULL;
    KBNODE pkb = get_pubkeyblock (keyid);

    if (!pkb)
      {
        err = -1;
        log_error ("oops: public key not found for preference check\n");
      }
    else if (pkb->pkt->pkt.public_key->selfsigversion > 3
             && dek->algo != CIPHER_ALGO_3DES
             && !opt.quiet
             && !is_algo_in_prefs (pkb, PREFTYPE_SYM, dek->algo))
      log_info (_("WARNING: cipher algorithm %s not found in recipient"
                  " preferences\n"), openpgp_cipher_algo_name (dek->algo));
    if (!err)
      {
        KBNODE k;

        for (k = pkb; k; k = k->next)
          {
            if (k->pkt->pkttype == PKT_PUBLIC_KEY
                || k->pkt->pkttype == PKT_PUBLIC_SUBKEY)
              {
                u32 aki[2];
                keyid_from_pk (k->pkt->pkt.public_key, aki);

                if (aki[0] == keyid[0] && aki[1] == keyid[1])
                  {
                    pk = k->pkt->pkt.public_key;
                    break;
                  }
              }
          }
        if (!pk)
          BUG ();
        if (pk->expiredate && pk->expiredate <= make_timestamp ())
          {
            log_info (_("Note: secret key %s expired at %s\n"),
                      keystr (keyid), asctimestamp (pk->expiredate));
          }
      }

    if (pk && pk->flags.revoked)
      {
        log_info (_("Note: key has been revoked"));
        log_printf ("\n");
        show_revocation_reason (pk, 1);
      }

    release_kbnode (pkb);
    err = 0;
  }

 leave:
  xfree (frame);
  xfree (keygrip);
  return err;
}