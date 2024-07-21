asn1_get_length(unsigned char **buffer,	/* IO - Pointer in buffer */
		unsigned char *bufend)	/* I  - End of buffer */
{
  unsigned	length;			/* Length */


  length = **buffer;
  (*buffer) ++;

  if (length & 128)
  {
    int	count;				/* Number of bytes for length */


    if ((count = length & 127) > sizeof(unsigned))
    {
      (*buffer) += count;
      return (0);
    }

    for (length = 0;
	 count > 0 && *buffer < bufend;
	 count --, (*buffer) ++)
      length = (length << 8) | **buffer;
  }

  return (length);
}