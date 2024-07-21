rend_decrypt_introduction_points(char **ipos_decrypted,
                                 size_t *ipos_decrypted_size,
                                 const char *descriptor_cookie,
                                 const char *ipos_encrypted,
                                 size_t ipos_encrypted_size)
{
  tor_assert(ipos_encrypted);
  tor_assert(descriptor_cookie);
  if (ipos_encrypted_size < 2) {
    log_warn(LD_REND, "Size of encrypted introduction points is too "
                      "small.");
    return -1;
  }
  if (ipos_encrypted[0] == (int)REND_BASIC_AUTH) {
    char iv[CIPHER_IV_LEN], client_id[REND_BASIC_AUTH_CLIENT_ID_LEN],
         session_key[CIPHER_KEY_LEN], *dec;
    int declen, client_blocks;
    size_t pos = 0, len, client_entries_len;
    crypto_digest_t *digest;
    crypto_cipher_t *cipher;
    client_blocks = (int) ipos_encrypted[1];
    client_entries_len = client_blocks * REND_BASIC_AUTH_CLIENT_MULTIPLE *
                         REND_BASIC_AUTH_CLIENT_ENTRY_LEN;
    if (ipos_encrypted_size < 2 + client_entries_len + CIPHER_IV_LEN + 1) {
      log_warn(LD_REND, "Size of encrypted introduction points is too "
                        "small.");
      return -1;
    }
    memcpy(iv, ipos_encrypted + 2 + client_entries_len, CIPHER_IV_LEN);
    digest = crypto_digest_new();
    crypto_digest_add_bytes(digest, descriptor_cookie, REND_DESC_COOKIE_LEN);
    crypto_digest_add_bytes(digest, iv, CIPHER_IV_LEN);
    crypto_digest_get_digest(digest, client_id,
                             REND_BASIC_AUTH_CLIENT_ID_LEN);
    crypto_digest_free(digest);
    for (pos = 2; pos < 2 + client_entries_len;
         pos += REND_BASIC_AUTH_CLIENT_ENTRY_LEN) {
      if (tor_memeq(ipos_encrypted + pos, client_id,
                  REND_BASIC_AUTH_CLIENT_ID_LEN)) {
        /* Attempt to decrypt introduction points. */
        cipher = crypto_cipher_new(descriptor_cookie);
        if (crypto_cipher_decrypt(cipher, session_key, ipos_encrypted
                                  + pos + REND_BASIC_AUTH_CLIENT_ID_LEN,
                                  CIPHER_KEY_LEN) < 0) {
          log_warn(LD_REND, "Could not decrypt session key for client.");
          crypto_cipher_free(cipher);
          return -1;
        }
        crypto_cipher_free(cipher);

        len = ipos_encrypted_size - 2 - client_entries_len - CIPHER_IV_LEN;
        dec = tor_malloc(len);
        declen = crypto_cipher_decrypt_with_iv(session_key, dec, len,
            ipos_encrypted + 2 + client_entries_len,
            ipos_encrypted_size - 2 - client_entries_len);

        if (declen < 0) {
          log_warn(LD_REND, "Could not decrypt introduction point string.");
          tor_free(dec);
          return -1;
        }
        if (fast_memcmpstart(dec, declen, "introduction-point ")) {
          log_warn(LD_REND, "Decrypted introduction points don't "
                            "look like we could parse them.");
          tor_free(dec);
          continue;
        }
        *ipos_decrypted = dec;
        *ipos_decrypted_size = declen;
        return 0;
      }
    }
    log_warn(LD_REND, "Could not decrypt introduction points. Please "
             "check your authorization for this service!");
    return -1;
  } else if (ipos_encrypted[0] == (int)REND_STEALTH_AUTH) {
    char *dec;
    int declen;
    if (ipos_encrypted_size < CIPHER_IV_LEN + 2) {
      log_warn(LD_REND, "Size of encrypted introduction points is too "
                        "small.");
      return -1;
    }
    dec = tor_malloc_zero(ipos_encrypted_size - CIPHER_IV_LEN - 1);

    declen = crypto_cipher_decrypt_with_iv(descriptor_cookie, dec,
                                           ipos_encrypted_size -
                                               CIPHER_IV_LEN - 1,
                                           ipos_encrypted + 1,
                                           ipos_encrypted_size - 1);

    if (declen < 0) {
      log_warn(LD_REND, "Decrypting introduction points failed!");
      tor_free(dec);
      return -1;
    }
    *ipos_decrypted = dec;
    *ipos_decrypted_size = declen;
    return 0;
  } else {
    log_warn(LD_REND, "Unknown authorization type number: %d",
             ipos_encrypted[0]);
    return -1;
  }
}