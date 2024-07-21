_asn1_get_objectid_der (const unsigned char *der, int der_len, int *ret_len,
			char *str, int str_size)
{
  int len_len, len, k;
  int leading;
  char temp[20];
  unsigned long val, val1;

  *ret_len = 0;
  if (str && str_size > 0)
    str[0] = 0;			/* no oid */

  if (str == NULL || der_len <= 0)
    return ASN1_GENERIC_ERROR;

  len = asn1_get_length_der (der, der_len, &len_len);

  if (len < 0 || len > der_len || len_len > der_len)
    return ASN1_DER_ERROR;

  val1 = der[len_len] / 40;
  val = der[len_len] - val1 * 40;

  _asn1_str_cpy (str, str_size, _asn1_ltostr (val1, temp));
  _asn1_str_cat (str, str_size, ".");
  _asn1_str_cat (str, str_size, _asn1_ltostr (val, temp));

  val = 0;
  leading = 1;
  for (k = 1; k < len; k++)
    {
      /* X.690 mandates that the leading byte must never be 0x80
       */
      if (leading != 0 && der[len_len + k] == 0x80)
	return ASN1_DER_ERROR;
      leading = 0;

      /* check for wrap around */
      if (INT_LEFT_SHIFT_OVERFLOW (val, 7))
	return ASN1_DER_ERROR;

      val = val << 7;
      val |= der[len_len + k] & 0x7F;

      if (!(der[len_len + k] & 0x80))
	{
	  _asn1_str_cat (str, str_size, ".");
	  _asn1_str_cat (str, str_size, _asn1_ltostr (val, temp));
	  val = 0;
	  leading = 1;
	}
    }

  if (INT_ADD_OVERFLOW (len, len_len))
    return ASN1_DER_ERROR;

  *ret_len = len + len_len;

  return ASN1_SUCCESS;
}