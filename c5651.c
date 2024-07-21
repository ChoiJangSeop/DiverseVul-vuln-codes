static int sha256_password_authenticate(MYSQL_PLUGIN_VIO *vio,
                                        MYSQL_SERVER_AUTH_INFO *info)
{
  uchar *pkt;
  int pkt_len;
  char  *user_salt_begin;
  char  *user_salt_end;
  char scramble[SCRAMBLE_LENGTH + 1];
  char stage2[CRYPT_MAX_PASSWORD_SIZE + 1];
  String scramble_response_packet;
#if !defined(HAVE_YASSL)
  int cipher_length= 0;
  unsigned char plain_text[MAX_CIPHER_LENGTH];
  RSA *private_key= NULL;
  RSA *public_key= NULL;
#endif

  DBUG_ENTER("sha256_password_authenticate");

  generate_user_salt(scramble, SCRAMBLE_LENGTH + 1);

  /*
    Note: The nonce is split into 8 + 12 bytes according to
http://dev.mysql.com/doc/internals/en/connection-phase-packets.html#packet-Protocol::HandshakeV10
    Native authentication sent 20 bytes + '\0' character = 21 bytes.
    This plugin must do the same to stay consistent with historical behavior 
    if it is set to operate as a default plugin.
  */
  scramble[SCRAMBLE_LENGTH] = '\0';
  if (vio->write_packet(vio, (unsigned char *) scramble, SCRAMBLE_LENGTH + 1))
    DBUG_RETURN(CR_ERROR);

  /*
    After the call to read_packet() the user name will appear in
    mpvio->acl_user and info will contain current data.
  */
  if ((pkt_len= vio->read_packet(vio, &pkt)) == -1)
    DBUG_RETURN(CR_ERROR);

  /*
    If first packet is a 0 byte then the client isn't sending any password
    else the client will send a password.

    The original intention was that the password is a string[NUL] but this
    never got enforced properly so now we have to accept that an empty packet
    is a blank password, thus the check for pkt_len == 0 has to be made too.
  */
  if ((pkt_len == 0 || pkt_len == 1) && *pkt == 0)
  {
    info->password_used= PASSWORD_USED_NO;
    /*
      Send OK signal; the authentication might still be rejected based on
      host mask.
    */
    if (info->auth_string_length == 0)
      DBUG_RETURN(CR_OK);
    else
      DBUG_RETURN(CR_ERROR);
  }
  else    
    info->password_used= PASSWORD_USED_YES;

  if (!my_vio_is_encrypted(vio))
  {
 #if !defined(HAVE_YASSL)
    /*
      Since a password is being used it must be encrypted by RSA since no 
      other encryption is being active.
    */
    private_key= g_rsa_keys.get_private_key();
    public_key=  g_rsa_keys.get_public_key();

    /*
      Without the keys encryption isn't possible.
    */
    if (private_key == NULL || public_key == NULL)
    {
      my_plugin_log_message(&plugin_info_ptr, MY_ERROR_LEVEL, 
        "Authentication requires either RSA keys or SSL encryption");
      DBUG_RETURN(CR_ERROR);
    }
      

    if ((cipher_length= g_rsa_keys.get_cipher_length()) > MAX_CIPHER_LENGTH)
    {
      my_plugin_log_message(&plugin_info_ptr, MY_ERROR_LEVEL, 
        "RSA key cipher length of %u is too long. Max value is %u.",
        g_rsa_keys.get_cipher_length(), MAX_CIPHER_LENGTH);
      DBUG_RETURN(CR_ERROR);
    }

    /*
      Client sent a "public key request"-packet ?
      If the first packet is 1 then the client will require a public key before
      encrypting the password.
    */
    if (pkt_len == 1 && *pkt == 1)
    {
      uint pem_length= strlen(g_rsa_keys.get_public_key_as_pem());
      if (vio->write_packet(vio,
                            (unsigned char *)g_rsa_keys.get_public_key_as_pem(),
                            pem_length))
        DBUG_RETURN(CR_ERROR);
      /* Get the encrypted response from the client */
      if ((pkt_len= vio->read_packet(vio, &pkt)) == -1)
        DBUG_RETURN(CR_ERROR);
    }

    /*
      The packet will contain the cipher used. The length of the packet
      must correspond to the expected cipher length.
    */
    if (pkt_len != cipher_length)
      DBUG_RETURN(CR_ERROR);
    
    /* Decrypt password */
    RSA_private_decrypt(cipher_length, pkt, plain_text, private_key,
                        RSA_PKCS1_OAEP_PADDING);

    plain_text[cipher_length]= '\0'; // safety
    xor_string((char *) plain_text, cipher_length,
               (char *) scramble, SCRAMBLE_LENGTH);

    /*
      Set packet pointers and length for the hash digest function below 
    */
    pkt= plain_text;
    pkt_len= strlen((char *) plain_text) + 1; // include \0 intentionally.

    if (pkt_len == 1)
      DBUG_RETURN(CR_ERROR);
#else
    DBUG_RETURN(CR_ERROR);
#endif
  } // if(!my_vio_is_encrypter())

  /* A password was sent to an account without a password */
  if (info->auth_string_length == 0)
    DBUG_RETURN(CR_ERROR);
  
  /*
    Fetch user authentication_string and extract the password salt
  */
  user_salt_begin= (char *) info->auth_string;
  user_salt_end= (char *) (info->auth_string + info->auth_string_length);
  if (extract_user_salt(&user_salt_begin, &user_salt_end) != CRYPT_SALT_LENGTH)
  {
    /* User salt is not correct */
    my_plugin_log_message(&plugin_info_ptr, MY_ERROR_LEVEL, 
      "Password salt for user '%s' is corrupt.",
      info->user_name);
    DBUG_RETURN(CR_ERROR);
  }

  /* Create hash digest */
  my_crypt_genhash(stage2,
                     CRYPT_MAX_PASSWORD_SIZE,
                     (char *) pkt,
                     pkt_len-1, 
                     (char *) user_salt_begin,
                     (const char **) 0);

  /* Compare the newly created hash digest with the password record */
  int result= memcmp(info->auth_string,
                     stage2,
                     info->auth_string_length);

  if (result == 0)
    DBUG_RETURN(CR_OK);

  DBUG_RETURN(CR_ERROR);
}