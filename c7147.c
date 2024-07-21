empe_app_smime_parse (EMailParserExtension *extension,
                      EMailParser *parser,
                      CamelMimePart *part,
                      GString *part_id,
                      GCancellable *cancellable,
                      GQueue *out_mail_parts)
{
	CamelCipherContext *context;
	CamelMimePart *opart;
	CamelCipherValidity *valid;
	CamelContentType *ct;
	GError *local_error = NULL;

	ct = camel_mime_part_get_content_type (part);
	if (camel_content_type_is (ct, "application", "pkcs7-signature") ||
	    camel_content_type_is (ct, "application", "xpkcs7-signature") ||
	    camel_content_type_is (ct, "application", "x-pkcs7-signature")) {
		return TRUE;
	}

	context = camel_smime_context_new (e_mail_parser_get_session (parser));

	opart = camel_mime_part_new ();
	valid = camel_cipher_context_decrypt_sync (
		context, part, opart,
		cancellable, &local_error);

	e_mail_part_preserve_charset_in_content_type (part, opart);

	if (local_error != NULL) {
		e_mail_parser_error (
			parser, out_mail_parts,
			_("Could not parse S/MIME message: %s"),
			local_error->message);
		g_error_free (local_error);

	} else {
		GQueue work_queue = G_QUEUE_INIT;
		GList *head, *link;
		gint len = part_id->len;

		g_string_append (part_id, ".encrypted-smime");

		e_mail_parser_parse_part (
			parser, opart, part_id, cancellable, &work_queue);

		g_string_truncate (part_id, len);

		head = g_queue_peek_head_link (&work_queue);

		/* Update validity flags of all the involved subp-arts */
		for (link = head; link != NULL; link = g_list_next (link)) {
			EMailPart *mail_part = link->data;

			e_mail_part_update_validity (
				mail_part, valid,
				E_MAIL_PART_VALIDITY_ENCRYPTED |
				E_MAIL_PART_VALIDITY_SMIME);
		}

		e_queue_transfer (&work_queue, out_mail_parts);

		/* Add a widget with details about the encryption, but only
		 * when the encrypted isn't itself secured, in that case it
		 * has created the button itself. */
		if (!e_mail_part_is_secured (opart)) {
			EMailPart *mail_part;

			g_string_append (part_id, ".encrypted-smime.button");

			e_mail_parser_parse_part_as (
				parser, part, part_id,
				"application/vnd.evolution.secure-button",
				cancellable, &work_queue);

			mail_part = g_queue_peek_head (&work_queue);

			if (mail_part != NULL)
				e_mail_part_update_validity (
					mail_part, valid,
					E_MAIL_PART_VALIDITY_ENCRYPTED |
					E_MAIL_PART_VALIDITY_SMIME);

			e_queue_transfer (&work_queue, out_mail_parts);

			g_string_truncate (part_id, len);
		}

		camel_cipher_validity_free (valid);
	}

	g_object_unref (opart);
	g_object_unref (context);

	return TRUE;
}