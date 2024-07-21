p12_build (gcry_mpi_t *kparms, const void *cert, size_t certlen,
           const char *pw, const char *charset, size_t *r_length)
{
  unsigned char *buffer = NULL;
  size_t n, buflen;
  char salt[8];
  struct buffer_s seqlist[3];
  int seqlistidx = 0;
  unsigned char sha1hash[20];
  char keyidstr[8+1];
  char *pwbuf = NULL;
  size_t pwbufsize = 0;

  n = buflen = 0; /* (avoid compiler warning). */
  memset (sha1hash, 0, 20);
  *keyidstr = 0;

  if (charset && pw && *pw)
    {
      jnlib_iconv_t cd;
      const char *inptr;
      char *outptr;
      size_t inbytes, outbytes;

      /* We assume that the converted passphrase is at max 2 times
         longer than its utf-8 encoding. */
      pwbufsize = strlen (pw)*2 + 1;
      pwbuf = gcry_malloc_secure (pwbufsize);
      if (!pwbuf)
        {
          log_error ("out of secure memory while converting passphrase\n");
          goto failure;
        }

      cd = jnlib_iconv_open (charset, "utf-8");
      if (cd == (jnlib_iconv_t)(-1))
        {
          log_error ("can't convert passphrase to"
                     " requested charset '%s': %s\n",
                     charset, strerror (errno));
          gcry_free (pwbuf);
          goto failure;
        }

      inptr = pw;
      inbytes = strlen (pw);
      outptr = pwbuf;
      outbytes = pwbufsize - 1;
      if ( jnlib_iconv (cd, (const char **)&inptr, &inbytes,
                      &outptr, &outbytes) == (size_t)-1)
        {
          log_error ("error converting passphrase to"
                     " requested charset '%s': %s\n",
                     charset, strerror (errno));
          gcry_free (pwbuf);
          jnlib_iconv_close (cd);
          goto failure;
        }
      *outptr = 0;
      jnlib_iconv_close (cd);
      pw = pwbuf;
    }


  if (cert && certlen)
    {
      /* Calculate the hash value we need for the bag attributes. */
      gcry_md_hash_buffer (GCRY_MD_SHA1, sha1hash, cert, certlen);
      sprintf (keyidstr, "%02x%02x%02x%02x",
               sha1hash[16], sha1hash[17], sha1hash[18], sha1hash[19]);

      /* Encode the certificate. */
      buffer = build_cert_sequence (cert, certlen, sha1hash, keyidstr,
                                    &buflen);
      if (!buffer)
        goto failure;

      /* Encrypt it. */
      gcry_randomize (salt, 8, GCRY_STRONG_RANDOM);
      crypt_block (buffer, buflen, salt, 8, 2048, NULL, 0, pw,
                   GCRY_CIPHER_RFC2268_40, 1);

      /* Encode the encrypted stuff into a bag. */
      seqlist[seqlistidx].buffer = build_cert_bag (buffer, buflen, salt, &n);
      seqlist[seqlistidx].length = n;
      gcry_free (buffer);
      buffer = NULL;
      if (!seqlist[seqlistidx].buffer)
        goto failure;
      seqlistidx++;
    }


  if (kparms)
    {
      /* Encode the key. */
      buffer = build_key_sequence (kparms, 0, &buflen);
      if (!buffer)
        goto failure;

      /* Encrypt it. */
      gcry_randomize (salt, 8, GCRY_STRONG_RANDOM);
      crypt_block (buffer, buflen, salt, 8, 2048, NULL, 0,
                   pw, GCRY_CIPHER_3DES, 1);

      /* Encode the encrypted stuff into a bag. */
      if (cert && certlen)
        seqlist[seqlistidx].buffer = build_key_bag (buffer, buflen, salt,
                                                    sha1hash, keyidstr, &n);
      else
        seqlist[seqlistidx].buffer = build_key_bag (buffer, buflen, salt,
                                                    NULL, NULL, &n);
      seqlist[seqlistidx].length = n;
      gcry_free (buffer);
      buffer = NULL;
      if (!seqlist[seqlistidx].buffer)
        goto failure;
      seqlistidx++;
    }

  seqlist[seqlistidx].buffer = NULL;
  seqlist[seqlistidx].length = 0;

  buffer = create_final (seqlist, pw, &buflen);

 failure:
  if (pwbuf)
    {
      wipememory (pwbuf, pwbufsize);
      gcry_free (pwbuf);
    }
  for ( ; seqlistidx; seqlistidx--)
    gcry_free (seqlist[seqlistidx].buffer);

  *r_length = buffer? buflen : 0;
  return buffer;
}