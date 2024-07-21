asn1_get_string(
    unsigned char **buffer,		/* IO - Pointer in buffer */
    unsigned char *bufend,		/* I  - End of buffer */
    unsigned      length,		/* I  - Value length */
    char          *string,		/* I  - String buffer */
    size_t        strsize)		/* I  - String buffer size */
{
  if (length > (unsigned)(bufend - *buffer))
    length = (unsigned)(bufend - *buffer);

  if (length < strsize)
  {
   /*
    * String is smaller than the buffer...
    */

    if (length > 0)
      memcpy(string, *buffer, length);

    string[length] = '\0';
  }
  else
  {
   /*
    * String is larger than the buffer...
    */

    memcpy(string, *buffer, strsize - 1);
    string[strsize - 1] = '\0';
  }

  if (length > 0)
    (*buffer) += length;

  return (string);
}