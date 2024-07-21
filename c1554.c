dane_verification_status_print (unsigned int status,
                       gnutls_datum_t * out, unsigned int flags)
{
  gnutls_buffer_st str;
  int ret;

  _gnutls_buffer_init (&str);

  if (status == 0)
    _gnutls_buffer_append_str (&str, _("Certificate matches. "));
  else
    _gnutls_buffer_append_str (&str, _("Verification failed. "));

  if (status & DANE_VERIFY_CA_CONSTRAINTS_VIOLATED)
    _gnutls_buffer_append_str (&str, _("CA constrains were violated. "));

  if (status & DANE_VERIFY_CERT_DIFFERS)
    _gnutls_buffer_append_str (&str, _("The certificate differs. "));

  if (status & DANE_VERIFY_NO_DANE_INFO)
    _gnutls_buffer_append_str (&str, _("There were no DANE information. "));

  ret = _gnutls_buffer_to_datum( &str, out);
  if (out->size > 0) out->size--;
      
  return ret;
}