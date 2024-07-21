_asn1_extract_der_octet (asn1_node node, const unsigned char *der,
			 int der_len)
{
  int len2, len3;
  int counter2, counter_end;

  len2 = asn1_get_length_der (der, der_len, &len3);
  if (len2 < -1)
    return ASN1_DER_ERROR;

  counter2 = len3 + 1;

  if (len2 == -1)
    counter_end = der_len - 2;
  else
    counter_end = der_len;

  while (counter2 < counter_end)
    {
      len2 = asn1_get_length_der (der + counter2, der_len - counter2, &len3);

      if (len2 < -1)
	return ASN1_DER_ERROR;

      if (len2 > 0)
	{
	  _asn1_append_value (node, der + counter2 + len3, len2);
	}
      else
	{			/* indefinite */

	  len2 =
	    _asn1_extract_der_octet (node, der + counter2 + len3,
				     der_len - counter2 - len3);
	  if (len2 < 0)
	    return len2;
	}

      counter2 += len2 + len3 + 1;
    }

  return ASN1_SUCCESS;
}