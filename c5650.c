static int calculate_password(String *str, char *buffer)
{
  DBUG_ASSERT(str);
  if (str->length() == 0) // PASSWORD('') returns ''
    return 0;
  
  int buffer_len= 0;
  THD *thd= current_thd;
  int old_passwords= 0;
  if (thd)
    old_passwords= thd->variables.old_passwords;
  
#if defined(HAVE_OPENSSL)
  if (old_passwords == 2)
  {
    my_make_scrambled_password(buffer, str->ptr(),
                               str->length());
    buffer_len= (int) strlen(buffer) + 1;
  }
  else
#endif
  if (old_passwords == 0)
  {
    my_make_scrambled_password_sha1(buffer, str->ptr(),
                                    str->length());
    buffer_len= SCRAMBLED_PASSWORD_CHAR_LENGTH;
  }
  else
  if (old_passwords == 1)
  {
    my_make_scrambled_password_323(buffer, str->ptr(),
                                   str->length());
    buffer_len= SCRAMBLED_PASSWORD_CHAR_LENGTH_323;
  }
  return buffer_len;
}