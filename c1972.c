_asn1_get_indefinite_length_string (const unsigned char *der, int *len)
{
  int len2, len3, counter, indefinite;
  unsigned long tag;
  unsigned char class;

  counter = indefinite = 0;

  while (1)
    {
      if ((*len) < counter)
	return ASN1_DER_ERROR;

      if ((der[counter] == 0) && (der[counter + 1] == 0))
	{
	  counter += 2;
	  indefinite--;
	  if (indefinite <= 0)
	    break;
	  else
	    continue;
	}

      if (asn1_get_tag_der
	  (der + counter, *len - counter, &class, &len2,
	   &tag) != ASN1_SUCCESS)
	return ASN1_DER_ERROR;
      if (counter + len2 > *len)
	return ASN1_DER_ERROR;
      counter += len2;
      len2 = asn1_get_length_der (der + counter, *len - counter, &len3);
      if (len2 < -1)
	return ASN1_DER_ERROR;
      if (len2 == -1)
	{
	  indefinite++;
	  counter += 1;
	}
      else
	{
	  counter += len2 + len3;
	}
    }

  *len = counter;
  return ASN1_SUCCESS;

}