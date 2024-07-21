e_mail_part_update_validity (EMailPart *part,
                             CamelCipherValidity *validity,
                             EMailPartValidityFlags validity_type)
{
	EMailPartValidityPair *pair;
	EMailPartValidityFlags mask;

	g_return_if_fail (E_IS_MAIL_PART (part));
	g_return_if_fail (validity != NULL);

	mask = E_MAIL_PART_VALIDITY_PGP | E_MAIL_PART_VALIDITY_SMIME;

	pair = mail_part_find_validity_pair (part, validity_type & mask);
	if (pair != NULL) {
		pair->validity_type |= validity_type;
		camel_cipher_validity_envelope (pair->validity, validity);
	} else {
		pair = g_new0 (EMailPartValidityPair, 1);
		pair->validity_type = validity_type;
		pair->validity = camel_cipher_validity_clone (validity);

		g_queue_push_tail (&part->validities, pair);
	}
}