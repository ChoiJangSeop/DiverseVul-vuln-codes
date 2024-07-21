coolkey_get_public_key_from_certificate(sc_pkcs15_card_t *p15card, sc_cardctl_coolkey_object_t *obj)
{
	sc_pkcs15_cert_info_t cert_info;
	sc_pkcs15_cert_t *cert_out = NULL;
	sc_pkcs15_pubkey_t *key = NULL;
	int r;

	cert_info.value.value = NULL;
	r = coolkey_get_certificate(p15card->card, obj, &cert_info.value);
	if (r < 0) {
		goto fail;
	}
	r = sc_pkcs15_read_certificate(p15card, &cert_info, &cert_out);
	if (r < 0) {
		goto fail;
	}
	key = cert_out->key;
	cert_out->key = NULL; /* adopt the key from the cert */
fail:
	if (cert_out) {
		sc_pkcs15_free_certificate(cert_out);
	}
	if (cert_info.value.value) {
		free(cert_info.value.value);
	}
	return key;
}