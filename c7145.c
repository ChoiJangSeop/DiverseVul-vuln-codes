empe_mp_encrypted_parse (EMailParserExtension *extension,
                         EMailParser *parser,
                         CamelMimePart *part,
                         GString *part_id,
                         GCancellable *cancellable,
                         GQueue *out_mail_parts)
{
	CamelCipherContext *context;
	const gchar *protocol;
	CamelMimePart *opart;
	CamelCipherValidity *valid;
	CamelMultipartEncrypted *mpe;
	GQueue work_queue = G_QUEUE_INIT;
	GList *head, *link;
	GError *local_error = NULL;
	gint len;

	mpe = (CamelMultipartEncrypted *) camel_medium_get_content ((CamelMedium *) part);
	if (!CAMEL_IS_MULTIPART_ENCRYPTED (mpe)) {
		e_mail_parser_error (
			parser, out_mail_parts,
			_("Could not parse MIME message. "
			"Displaying as source."));
		e_mail_parser_parse_part_as (
			parser, part, part_id,
			"application/vnd.evolution/source",
			cancellable, out_mail_parts);

		return TRUE;
	}

	/* Currently we only handle RFC2015-style PGP encryption. */
	protocol = camel_content_type_param (camel_data_wrapper_get_mime_type_field (CAMEL_DATA_WRAPPER (mpe)), "protocol");
	if (!protocol || g_ascii_strcasecmp (protocol, "application/pgp-encrypted") != 0) {
		e_mail_parser_error (
			parser, out_mail_parts,
			_("Unsupported encryption type for multipart/encrypted"));
		e_mail_parser_parse_part_as (
			parser, part, part_id, "multipart/mixed",
			cancellable, out_mail_parts);

		return TRUE;
	}

	context = camel_gpg_context_new (e_mail_parser_get_session (parser));

	opart = camel_mime_part_new ();
	valid = camel_cipher_context_decrypt_sync (
		context, part, opart, cancellable, &local_error);

	e_mail_part_preserve_charset_in_content_type (part, opart);

	if (local_error != NULL) {
		e_mail_parser_error (
			parser, out_mail_parts,
			_("Could not parse PGP/MIME message: %s"),
			local_error->message);
		e_mail_parser_parse_part_as (
			parser, part, part_id, "multipart/mixed",
			cancellable, out_mail_parts);

		g_object_unref (opart);
		g_object_unref (context);
		g_error_free (local_error);

		return TRUE;
	}

	len = part_id->len;
	g_string_append (part_id, ".encrypted-pgp");

	g_warn_if_fail (e_mail_parser_parse_part (
		parser, opart, part_id, cancellable, &work_queue));

	g_string_truncate (part_id, len);

	head = g_queue_peek_head_link (&work_queue);

	/* Update validity of all encrypted sub-parts */
	for (link = head; link != NULL; link = g_list_next (link)) {
		EMailPart *mail_part = link->data;

		e_mail_part_update_validity (
			mail_part, valid,
			E_MAIL_PART_VALIDITY_ENCRYPTED |
			E_MAIL_PART_VALIDITY_PGP);
	}

	e_queue_transfer (&work_queue, out_mail_parts);

	/* Add a widget with details about the encryption, but only when
	 * the decrypted part isn't itself secured, in that case it has
	 * created the button itself. */
	if (!e_mail_part_is_secured (opart)) {
		EMailPart *mail_part;

		g_string_append (part_id, ".encrypted-pgp.button");

		e_mail_parser_parse_part_as (
			parser, part, part_id,
			"application/vnd.evolution.secure-button",
			cancellable, &work_queue);

		mail_part = g_queue_peek_head (&work_queue);

		if (mail_part != NULL)
			e_mail_part_update_validity (
				mail_part, valid,
				E_MAIL_PART_VALIDITY_ENCRYPTED |
				E_MAIL_PART_VALIDITY_PGP);

		e_queue_transfer (&work_queue, out_mail_parts);

		g_string_truncate (part_id, len);
	}

	camel_cipher_validity_free (valid);

	/* TODO: Make sure when we finalize this part, it is zero'd out */
	g_object_unref (opart);
	g_object_unref (context);

	return TRUE;
}