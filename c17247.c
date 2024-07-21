bool SafeX509ExtPrint(const BIOPointer& out, X509_EXTENSION* ext) {
  const X509V3_EXT_METHOD* method = X509V3_EXT_get(ext);

  if (method != X509V3_EXT_get_nid(NID_subject_alt_name))
    return false;

  GENERAL_NAMES* names = static_cast<GENERAL_NAMES*>(X509V3_EXT_d2i(ext));
  if (names == nullptr)
    return false;

  for (int i = 0; i < sk_GENERAL_NAME_num(names); i++) {
    GENERAL_NAME* gen = sk_GENERAL_NAME_value(names, i);

    if (i != 0)
      BIO_write(out.get(), ", ", 2);

    if (gen->type == GEN_DNS) {
      ASN1_IA5STRING* name = gen->d.dNSName;

      BIO_write(out.get(), "DNS:", 4);
      BIO_write(out.get(), name->data, name->length);
    } else {
      STACK_OF(CONF_VALUE)* nval = i2v_GENERAL_NAME(
          const_cast<X509V3_EXT_METHOD*>(method), gen, nullptr);
      if (nval == nullptr)
        return false;
      X509V3_EXT_val_prn(out.get(), nval, 0, 0);
      sk_CONF_VALUE_pop_free(nval, X509V3_conf_free);
    }
  }
  sk_GENERAL_NAME_pop_free(names, GENERAL_NAME_free);

  return true;
}