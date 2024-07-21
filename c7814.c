asn1_get_packed(
    unsigned char **buffer,		/* IO - Pointer in buffer */
    unsigned char *bufend)		/* I  - End of buffer */
{
  int	value;				/* Value */


  value = 0;

  while ((**buffer & 128) && *buffer < bufend)
  {
    value = (value << 7) | (**buffer & 127);
    (*buffer) ++;
  }

  if (*buffer < bufend)
  {
    value = (value << 7) | **buffer;
    (*buffer) ++;
  }

  return (value);
}