do_unprotect (const char *passphrase,
              int pkt_version, int pubkey_algo, int is_protected,
              const char *curve, gcry_mpi_t *skey, size_t skeysize,
              int protect_algo, void *protect_iv, size_t protect_ivlen,
              int s2k_mode, int s2k_algo, byte *s2k_salt, u32 s2k_count,
              u16 desired_csum, gcry_sexp_t *r_key)
{
  gpg_error_t err;
  unsigned int npkey, nskey, skeylen;
  gcry_cipher_hd_t cipher_hd = NULL;
  u16 actual_csum;
  size_t nbytes;
  int i;
  gcry_mpi_t tmpmpi;

  *r_key = NULL;

  err = prepare_unprotect (pubkey_algo, skey, skeysize, s2k_mode,
                           &npkey, &nskey, &skeylen);
  if (err)
    return err;

  /* Check whether SKEY is at all protected.  If it is not protected
     merely verify the checksum.  */
  if (!is_protected)
    {
      actual_csum = 0;
      for (i=npkey; i < nskey; i++)
        {
          if (!skey[i] || gcry_mpi_get_flag (skey[i], GCRYMPI_FLAG_USER1))
            return gpg_error (GPG_ERR_BAD_SECKEY);

          if (gcry_mpi_get_flag (skey[i], GCRYMPI_FLAG_OPAQUE))
            {
              unsigned int nbits;
              const unsigned char *buffer;
              buffer = gcry_mpi_get_opaque (skey[i], &nbits);
              nbytes = (nbits+7)/8;
              actual_csum += checksum (buffer, nbytes);
            }
          else
            {
              unsigned char *buffer;

              err = gcry_mpi_aprint (GCRYMPI_FMT_PGP, &buffer, &nbytes,
                                     skey[i]);
              if (!err)
                actual_csum += checksum (buffer, nbytes);
              xfree (buffer);
            }
          if (err)
            return err;
        }

      if (actual_csum != desired_csum)
        return gpg_error (GPG_ERR_CHECKSUM);

      goto do_convert;
    }


  if (gcry_cipher_test_algo (protect_algo))
    {
      /* The algorithm numbers are Libgcrypt numbers but fortunately
         the OpenPGP algorithm numbers map one-to-one to the Libgcrypt
         numbers.  */
      log_info (_("protection algorithm %d (%s) is not supported\n"),
                protect_algo, gnupg_cipher_algo_name (protect_algo));
      return gpg_error (GPG_ERR_CIPHER_ALGO);
    }

  if (gcry_md_test_algo (s2k_algo))
    {
      log_info (_("protection hash algorithm %d (%s) is not supported\n"),
                s2k_algo, gcry_md_algo_name (s2k_algo));
      return gpg_error (GPG_ERR_DIGEST_ALGO);
    }

  err = gcry_cipher_open (&cipher_hd, protect_algo,
                          GCRY_CIPHER_MODE_CFB,
                          (GCRY_CIPHER_SECURE
                           | (protect_algo >= 100 ?
                              0 : GCRY_CIPHER_ENABLE_SYNC)));
  if (err)
    {
      log_error ("failed to open cipher_algo %d: %s\n",
                 protect_algo, gpg_strerror (err));
      return err;
    }

  err = hash_passphrase_and_set_key (passphrase, cipher_hd, protect_algo,
                                     s2k_mode, s2k_algo, s2k_salt, s2k_count);
  if (err)
    {
      gcry_cipher_close (cipher_hd);
      return err;
    }

  gcry_cipher_setiv (cipher_hd, protect_iv, protect_ivlen);

  actual_csum = 0;
  if (pkt_version >= 4)
    {
      int ndata;
      unsigned int ndatabits;
      const unsigned char *p;
      unsigned char *data;
      u16 csum_pgp7 = 0;

      if (!gcry_mpi_get_flag (skey[npkey], GCRYMPI_FLAG_OPAQUE ))
        {
          gcry_cipher_close (cipher_hd);
          return gpg_error (GPG_ERR_BAD_SECKEY);
        }
      p = gcry_mpi_get_opaque (skey[npkey], &ndatabits);
      ndata = (ndatabits+7)/8;

      if (ndata > 1)
        csum_pgp7 = p[ndata-2] << 8 | p[ndata-1];
      data = xtrymalloc_secure (ndata);
      if (!data)
        {
          err = gpg_error_from_syserror ();
          gcry_cipher_close (cipher_hd);
          return err;
        }
      gcry_cipher_decrypt (cipher_hd, data, ndata, p, ndata);

      p = data;
      if (is_protected == 2)
        {
          /* This is the new SHA1 checksum method to detect tampering
             with the key as used by the Klima/Rosa attack.  */
          desired_csum = 0;
          actual_csum = 1;  /* Default to bad checksum.  */

          if (ndata < 20)
            log_error ("not enough bytes for SHA-1 checksum\n");
          else
            {
              gcry_md_hd_t h;

              if (gcry_md_open (&h, GCRY_MD_SHA1, 1))
                BUG(); /* Algo not available. */
              gcry_md_write (h, data, ndata - 20);
              gcry_md_final (h);
              if (!memcmp (gcry_md_read (h, GCRY_MD_SHA1), data+ndata-20, 20))
                actual_csum = 0; /* Digest does match.  */
              gcry_md_close (h);
            }
        }
      else
        {
          /* Old 16 bit checksum method.  */
          if (ndata < 2)
            {
              log_error ("not enough bytes for checksum\n");
              desired_csum = 0;
              actual_csum = 1;  /* Mark checksum bad.  */
            }
          else
            {
              desired_csum = (data[ndata-2] << 8 | data[ndata-1]);
              actual_csum = checksum (data, ndata-2);
              if (desired_csum != actual_csum)
                {
                  /* This is a PGP 7.0.0 workaround */
                  desired_csum = csum_pgp7; /* Take the encrypted one.  */
                }
            }
        }

      /* Better check it here.  Otherwise the gcry_mpi_scan would fail
         because the length may have an arbitrary value.  */
      if (desired_csum == actual_csum)
        {
          for (i=npkey; i < nskey; i++ )
            {
              if (gcry_mpi_scan (&tmpmpi, GCRYMPI_FMT_PGP, p, ndata, &nbytes))
                {
                  /* Checksum was okay, but not correctly decrypted.  */
                  desired_csum = 0;
                  actual_csum = 1;   /* Mark checksum bad.  */
                  break;
                }
              gcry_mpi_release (skey[i]);
              skey[i] = tmpmpi;
              ndata -= nbytes;
              p += nbytes;
            }
          skey[i] = NULL;
          skeylen = i;
          assert (skeylen <= skeysize);

          /* Note: at this point NDATA should be 2 for a simple
             checksum or 20 for the sha1 digest.  */
        }
      xfree(data);
    }
  else /* Packet version <= 3.  */
    {
      unsigned char *buffer;

      for (i = npkey; i < nskey; i++)
        {
          const unsigned char *p;
          size_t ndata;
          unsigned int ndatabits;

          if (!skey[i] || !gcry_mpi_get_flag (skey[i], GCRYMPI_FLAG_OPAQUE))
            {
              gcry_cipher_close (cipher_hd);
              return gpg_error (GPG_ERR_BAD_SECKEY);
            }
          p = gcry_mpi_get_opaque (skey[i], &ndatabits);
          ndata = (ndatabits+7)/8;

          if (!(ndata >= 2) || !(ndata == ((p[0] << 8 | p[1]) + 7)/8 + 2))
            {
              gcry_cipher_close (cipher_hd);
              return gpg_error (GPG_ERR_BAD_SECKEY);
            }

          buffer = xtrymalloc_secure (ndata);
          if (!buffer)
            {
              err = gpg_error_from_syserror ();
              gcry_cipher_close (cipher_hd);
              return err;
            }

          gcry_cipher_sync (cipher_hd);
          buffer[0] = p[0];
          buffer[1] = p[1];
          gcry_cipher_decrypt (cipher_hd, buffer+2, ndata-2, p+2, ndata-2);
          actual_csum += checksum (buffer, ndata);
          err = gcry_mpi_scan (&tmpmpi, GCRYMPI_FMT_PGP, buffer, ndata, &ndata);
          xfree (buffer);
          if (err)
            {
              /* Checksum was okay, but not correctly decrypted.  */
              desired_csum = 0;
              actual_csum = 1;   /* Mark checksum bad.  */
              break;
            }
          gcry_mpi_release (skey[i]);
          skey[i] = tmpmpi;
        }
    }
  gcry_cipher_close (cipher_hd);

  /* Now let's see whether we have used the correct passphrase. */
  if (actual_csum != desired_csum)
    return gpg_error (GPG_ERR_BAD_PASSPHRASE);

 do_convert:
  if (nskey != skeylen)
    err = gpg_error (GPG_ERR_BAD_SECKEY);
  else
    err = convert_secret_key (r_key, pubkey_algo, skey, curve);
  if (err)
    return err;

  /* The checksum may fail, thus we also check the key itself.  */
  err = gcry_pk_testkey (*r_key);
  if (err)
    {
      gcry_sexp_release (*r_key);
      *r_key = NULL;
      return gpg_error (GPG_ERR_BAD_PASSPHRASE);
    }

  return 0;
}