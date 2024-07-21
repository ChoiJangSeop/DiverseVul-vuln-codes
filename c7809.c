asn1_get_oid(
    unsigned char **buffer,		/* IO - Pointer in buffer */
    unsigned char *bufend,		/* I  - End of buffer */
    unsigned      length,		/* I  - Length of value */
    int           *oid,			/* I  - OID buffer */
    int           oidsize)		/* I  - Size of OID buffer */
{
  unsigned char	*valend;		/* End of value */
  int		*oidptr,		/* Current OID */
		*oidend;		/* End of OID buffer */
  int		number;			/* OID number */


  valend = *buffer + length;
  oidptr = oid;
  oidend = oid + oidsize - 1;

  if (valend > bufend)
    valend = bufend;

  number = asn1_get_packed(buffer, bufend);

  if (number < 80)
  {
    *oidptr++ = number / 40;
    number    = number % 40;
    *oidptr++ = number;
  }
  else
  {
    *oidptr++ = 2;
    number    -= 80;
    *oidptr++ = number;
  }

  while (*buffer < valend)
  {
    number = asn1_get_packed(buffer, bufend);

    if (oidptr < oidend)
      *oidptr++ = number;
  }

  *oidptr = -1;

  return ((int)(oidptr - oid));
}