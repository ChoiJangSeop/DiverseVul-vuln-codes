asn1_get_type(unsigned char **buffer,	/* IO - Pointer in buffer */
	      unsigned char *bufend)	/* I  - End of buffer */
{
  int	type;				/* Type */


  type = **buffer;
  (*buffer) ++;

  if ((type & 31) == 31)
    type = asn1_get_packed(buffer, bufend);

  return (type);
}