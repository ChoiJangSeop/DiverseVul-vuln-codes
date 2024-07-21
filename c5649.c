int digest_password(THD *thd, LEX_USER *user_record)
{
  /* Empty passwords stay empty */
  if (user_record->password.length == 0)
    return 0;

#if defined(HAVE_OPENSSL)
  /*
    Transform password into a password hash 
  */
  if (user_record->plugin.str == sha256_password_plugin_name.str)
  {
    char *buff=  (char *) thd->alloc(CRYPT_MAX_PASSWORD_SIZE+1);
    if (buff == NULL)
      return 1;

    my_make_scrambled_password(buff, user_record->password.str,
                               user_record->password.length);
    user_record->password.str= buff;
    user_record->password.length= strlen(buff)+1;
  }
  else
#endif
  if (user_record->plugin.str == native_password_plugin_name.str ||
      user_record->plugin.str == old_password_plugin_name.str)
  {
    if (thd->variables.old_passwords == 1)
    {
      char *buff= 
        (char *) thd->alloc(SCRAMBLED_PASSWORD_CHAR_LENGTH_323+1);
      if (buff == NULL)
        return 1;

      my_make_scrambled_password_323(buff, user_record->password.str,
                                     user_record->password.length);
      user_record->password.str= buff;
      user_record->password.length= SCRAMBLED_PASSWORD_CHAR_LENGTH_323;
    }
    else
    {
      char *buff= 
        (char *) thd->alloc(SCRAMBLED_PASSWORD_CHAR_LENGTH+1);
      if (buff == NULL)
        return 1;

      my_make_scrambled_password_sha1(buff, user_record->password.str,
                                      user_record->password.length);
      user_record->password.str= buff;
      user_record->password.length= SCRAMBLED_PASSWORD_CHAR_LENGTH;
    }
  } // end if native_password_plugin_name || old_password_plugin_name
  else
  {
    user_record->password.str= 0;
    user_record->password.length= 0;
  }
  return 0;
}