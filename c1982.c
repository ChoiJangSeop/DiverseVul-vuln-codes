_asn1_get_time_der (const unsigned char *der, int der_len, int *ret_len,
		    char *str, int str_size)
{
  int len_len, str_len;

  if (der_len <= 0 || str == NULL)
    return ASN1_DER_ERROR;
  str_len = asn1_get_length_der (der, der_len, &len_len);
  if (str_len < 0 || str_size < str_len)
    return ASN1_DER_ERROR;
  memcpy (str, der + len_len, str_len);
  str[str_len] = 0;
  *ret_len = str_len + len_len;

  return ASN1_SUCCESS;
}