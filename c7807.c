asn1_get_integer(
    unsigned char **buffer,		/* IO - Pointer in buffer */
    unsigned char *bufend,		/* I  - End of buffer */
    unsigned      length)		/* I  - Length of value */
{
  int	value;				/* Integer value */


  if (length > sizeof(int))
  {
    (*buffer) += length;
    return (0);
  }

  for (value = (**buffer & 0x80) ? -1 : 0;
       length > 0 && *buffer < bufend;
       length --, (*buffer) ++)
    value = (value << 8) | **buffer;

  return (value);
}