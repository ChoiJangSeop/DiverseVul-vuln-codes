asn1_get_tag_der (const unsigned char *der, int der_len,
		  unsigned char *cls, int *len, unsigned long *tag)
{
  unsigned int ris;
  int punt;

  if (der == NULL || der_len < 2 || len == NULL)
    return ASN1_DER_ERROR;

  *cls = der[0] & 0xE0;
  if ((der[0] & 0x1F) != 0x1F)
    {
      /* short form */
      *len = 1;
      ris = der[0] & 0x1F;
    }
  else
    {
      /* Long form */
      punt = 1;
      ris = 0;
      while (punt <= der_len && der[punt] & 128)
	{

	  if (INT_MULTIPLY_OVERFLOW (ris, 128))
	    return ASN1_DER_ERROR;
	  ris *= 128;

	  if (INT_ADD_OVERFLOW (ris, ((unsigned) (der[punt] & 0x7F))))
	    return ASN1_DER_ERROR;
	  ris += (der[punt] & 0x7F);
	  punt++;
	}

      if (punt >= der_len)
	return ASN1_DER_ERROR;

      if (INT_MULTIPLY_OVERFLOW (ris, 128))
	return ASN1_DER_ERROR;
      ris *= 128;

      if (INT_ADD_OVERFLOW (ris, ((unsigned) (der[punt] & 0x7F))))
	return ASN1_DER_ERROR;
      ris += (der[punt] & 0x7F);
      punt++;

      *len = punt;
    }

  if (tag)
    *tag = ris;
  return ASN1_SUCCESS;
}