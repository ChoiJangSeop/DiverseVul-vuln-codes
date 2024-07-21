char *get_56_lenc_string(char **buffer,
                         size_t *max_bytes_available,
                         size_t *string_length)
{
  static char empty_string[1]= { '\0' };
  char *begin= *buffer;

  if (*max_bytes_available == 0)
    return NULL;

  /*
    If the length encoded string has the length 0
    the total size of the string is only one byte long (the size byte)
  */
  if (*begin == 0)
  {
    *string_length= 0;
    --*max_bytes_available;
    ++*buffer;
    /*
      Return a pointer to the \0 character so the return value will be
      an empty string.
    */
    return empty_string;
  }

  *string_length= (size_t)net_field_length_ll((uchar **)buffer);

  size_t len_len= (size_t)(*buffer - begin);
  
  if (*string_length + len_len > *max_bytes_available)
    return NULL;

  *max_bytes_available -= *string_length + len_len;
  *buffer += *string_length;
  return (char *)(begin + len_len);
}