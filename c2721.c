int sha256_password_auth_client(MYSQL_PLUGIN_VIO *vio, MYSQL *mysql)
{
  bool uses_password= mysql->passwd[0] != 0;
#if !defined(HAVE_YASSL)
  unsigned char encrypted_password[MAX_CIPHER_LENGTH];
  static char request_public_key= '\1';
  RSA *public_key= NULL;
  bool got_public_key_from_server= false;
#endif
  bool connection_is_secure= false;
  unsigned char scramble_pkt[20];
  unsigned char *pkt;


  DBUG_ENTER("sha256_password_auth_client");

  /*
    Get the scramble from the server because we need it when sending encrypted
    password.
  */
  if (vio->read_packet(vio, &pkt) != SCRAMBLE_LENGTH)
  {
    DBUG_PRINT("info",("Scramble is not of correct length."));
    DBUG_RETURN(CR_ERROR);
  }
  /*
    Copy the scramble to the stack or it will be lost on the next use of the 
    net buffer.
  */
  memcpy(scramble_pkt, pkt, SCRAMBLE_LENGTH);

  if (mysql_get_ssl_cipher(mysql) != NULL)
    connection_is_secure= true;
  
  /* If connection isn't secure attempt to get the RSA public key file */
  if (!connection_is_secure)
  {
 #if !defined(HAVE_YASSL)
    public_key= rsa_init(mysql);
#endif
  }

  if (!uses_password)
  {
    /* We're not using a password */
    static const unsigned char zero_byte= '\0'; 
    if (vio->write_packet(vio, (const unsigned char *) &zero_byte, 1))
      DBUG_RETURN(CR_ERROR);
  }
  else
  {
    /* Password is a 0-terminated byte array ('\0' character included) */
    unsigned int passwd_len= strlen(mysql->passwd) + 1;
    if (!connection_is_secure)
    {
#if !defined(HAVE_YASSL)
      /*
        If no public key; request one from the server.
      */
      if (public_key == NULL)
      {
        if (vio->write_packet(vio, (const unsigned char *) &request_public_key,
                              1))
          DBUG_RETURN(CR_ERROR);
      
        int pkt_len= 0;
        unsigned char *pkt;
        if ((pkt_len= vio->read_packet(vio, &pkt)) == -1)
          DBUG_RETURN(CR_ERROR);
        BIO* bio= BIO_new_mem_buf(pkt, pkt_len);
        public_key= PEM_read_bio_RSA_PUBKEY(bio, NULL, NULL, NULL);
        BIO_free(bio);
        if (public_key == 0)
        {
          ERR_clear_error();
          DBUG_RETURN(CR_ERROR);
        }
        got_public_key_from_server= true;
      }
      
      /* Obfuscate the plain text password with the session scramble */
      xor_string(mysql->passwd, strlen(mysql->passwd), (char *) scramble_pkt,
                 SCRAMBLE_LENGTH);
      /* Encrypt the password and send it to the server */
      int cipher_length= RSA_size(public_key);
      /*
        When using RSA_PKCS1_OAEP_PADDING the password length must be less
        than RSA_size(rsa) - 41.
      */
      if (passwd_len + 41 >= (unsigned) cipher_length)
      {
        /* password message is to long */
        DBUG_RETURN(CR_ERROR);
      }
      RSA_public_encrypt(passwd_len, (unsigned char *) mysql->passwd,
                         encrypted_password,
                         public_key, RSA_PKCS1_OAEP_PADDING);
      if (got_public_key_from_server)
        RSA_free(public_key);

      if (vio->write_packet(vio, (uchar*) encrypted_password, cipher_length))
        DBUG_RETURN(CR_ERROR);
#else
      set_mysql_extended_error(mysql, CR_AUTH_PLUGIN_ERR, unknown_sqlstate,
                                ER(CR_AUTH_PLUGIN_ERR), "sha256_password",
                                "Authentication requires SSL encryption");
      DBUG_RETURN(CR_ERROR); // If no openssl support
#endif
    }
    else
    {
      /* The vio is encrypted already; just send the plain text passwd */
      if (vio->write_packet(vio, (uchar*) mysql->passwd, passwd_len))
        DBUG_RETURN(CR_ERROR);
    }
    
    memset(mysql->passwd, 0, passwd_len);
  }
    
  DBUG_RETURN(CR_OK);
}