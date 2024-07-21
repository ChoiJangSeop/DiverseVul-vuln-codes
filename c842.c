compressed_to_ciphertext (gnutls_session_t session,
                               uint8_t * cipher_data, int cipher_size,
                               gnutls_datum_t *compressed,
                               content_type_t type, 
                               record_parameters_st * params)
{
  uint8_t * tag_ptr = NULL;
  uint8_t pad;
  int length, length_to_encrypt, ret;
  uint8_t preamble[MAX_PREAMBLE_SIZE];
  int preamble_size;
  int tag_size = _gnutls_auth_cipher_tag_len (&params->write.cipher_state);
  int blocksize = gnutls_cipher_get_block_size (params->cipher_algorithm);
  unsigned block_algo =
    _gnutls_cipher_is_block (params->cipher_algorithm);
  uint8_t *data_ptr;
  int ver = gnutls_protocol_get_version (session);
  int explicit_iv = _gnutls_version_has_explicit_iv (session->security_parameters.version);
  int auth_cipher = _gnutls_auth_cipher_is_aead(&params->write.cipher_state);
  int random_pad;
  
  /* We don't use long padding if requested or if we are in DTLS.
   */
  if (session->internals.priorities.no_padding == 0 && (!IS_DTLS(session)))
    random_pad = 1;
  else
    random_pad = 0;
  
  _gnutls_hard_log("ENC[%p]: cipher: %s, MAC: %s, Epoch: %u\n",
    session, gnutls_cipher_get_name(params->cipher_algorithm), gnutls_mac_get_name(params->mac_algorithm),
    (unsigned int)params->epoch);

  preamble_size =
    make_preamble (UINT64DATA
                   (params->write.sequence_number),
                   type, compressed->size, ver, preamble);

  /* Calculate the encrypted length (padding etc.)
   */
  length_to_encrypt = length =
    calc_enc_length (session, compressed->size, tag_size, &pad,
                     random_pad, block_algo, auth_cipher, blocksize);
  if (length < 0)
    {
      return gnutls_assert_val(length);
    }

  /* copy the encrypted data to cipher_data.
   */
  if (cipher_size < length)
    {
      return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
    }

  data_ptr = cipher_data;

  if (explicit_iv)
    {

      if (block_algo == CIPHER_BLOCK)
        {
          /* copy the random IV.
           */
          ret = _gnutls_rnd (GNUTLS_RND_NONCE, data_ptr, blocksize);
          if (ret < 0)
            return gnutls_assert_val(ret);

          _gnutls_auth_cipher_setiv(&params->write.cipher_state, data_ptr, blocksize);

          data_ptr += blocksize;
          cipher_data += blocksize;
          length_to_encrypt -= blocksize;
        }
      else if (auth_cipher)
        {
          uint8_t nonce[blocksize];

          /* Values in AEAD are pretty fixed in TLS 1.2 for 128-bit block
           */
          if (params->write.IV.data == NULL || params->write.IV.size != AEAD_IMPLICIT_DATA_SIZE)
            return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

          /* Instead of generating a new nonce on every packet, we use the
           * write.sequence_number (It is a MAY on RFC 5288).
           */
          memcpy(nonce, params->write.IV.data, params->write.IV.size);
          memcpy(&nonce[AEAD_IMPLICIT_DATA_SIZE], UINT64DATA(params->write.sequence_number), 8);

          _gnutls_auth_cipher_setiv(&params->write.cipher_state, nonce, AEAD_IMPLICIT_DATA_SIZE+AEAD_EXPLICIT_DATA_SIZE);

          /* copy the explicit part */
          memcpy(data_ptr, &nonce[AEAD_IMPLICIT_DATA_SIZE], AEAD_EXPLICIT_DATA_SIZE);

          data_ptr += AEAD_EXPLICIT_DATA_SIZE;
          cipher_data += AEAD_EXPLICIT_DATA_SIZE;
          /* In AEAD ciphers we don't encrypt the tag 
           */
          length_to_encrypt -= AEAD_EXPLICIT_DATA_SIZE + tag_size;
        }
    }
  else
    {
      /* AEAD ciphers have an explicit IV. Shouldn't be used otherwise.
       */
      if (auth_cipher) return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
    }

  memcpy (data_ptr, compressed->data, compressed->size);
  data_ptr += compressed->size;

  if (tag_size > 0)
    {
      tag_ptr = data_ptr;
      data_ptr += tag_size;
    }
  if (block_algo == CIPHER_BLOCK && pad > 0)
    {
      memset (data_ptr, pad - 1, pad);
    }

  /* add the authenticate data */
  ret = _gnutls_auth_cipher_add_auth(&params->write.cipher_state, preamble, preamble_size);
  if (ret < 0)
    return gnutls_assert_val(ret);

  /* Actual encryption (inplace).
   */
  ret =
    _gnutls_auth_cipher_encrypt2_tag (&params->write.cipher_state,
        cipher_data, length_to_encrypt, 
        cipher_data, cipher_size,
        tag_ptr, tag_size, compressed->size);
  if (ret < 0)
    return gnutls_assert_val(ret);

  return length;
}