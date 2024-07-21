pk_transaction_obtain_authorization (PkTransaction *transaction,
				     PkRoleEnum role,
				     GError **error)
{
	const gchar *action_id;
	const gchar *text;
	PkTransactionPrivate *priv = transaction->priv;
	_cleanup_free_ gchar *package_ids = NULL;
	_cleanup_object_unref_ PolkitDetails *details = NULL;
	_cleanup_string_free_ GString *string = NULL;

	g_return_val_if_fail (priv->sender != NULL, FALSE);

	/* we don't need to authenticate at all to just download
	 * packages or if we're running unit tests */
	if (pk_bitfield_contain (transaction->priv->cached_transaction_flags,
				 PK_TRANSACTION_FLAG_ENUM_ONLY_DOWNLOAD) ||
	    pk_bitfield_contain (transaction->priv->cached_transaction_flags,
				 PK_TRANSACTION_FLAG_ENUM_SIMULATE) ||
	    priv->skip_auth_check == TRUE) {
		g_debug ("No authentication required");
		pk_transaction_set_state (transaction, PK_TRANSACTION_STATE_READY);
		return TRUE;
	}

	/* we should always have subject */
	if (priv->subject == NULL) {
		g_set_error (error,
			     PK_TRANSACTION_ERROR,
			     PK_TRANSACTION_ERROR_REFUSED_BY_POLICY,
			     "subject %s not found", priv->sender);
		return FALSE;
	}

	/* map the roles to policykit rules */
	if (pk_bitfield_contain (transaction->priv->cached_transaction_flags,
				 PK_TRANSACTION_FLAG_ENUM_ONLY_TRUSTED)) {
		action_id = pk_transaction_role_to_action_only_trusted (role);
	} else {
		action_id = pk_transaction_role_to_action_allow_untrusted (role);
	}

	/* log */
	syslog (LOG_AUTH | LOG_INFO,
		"uid %i is trying to obtain %s auth (only_trusted:%i)",
		priv->uid,
		action_id,
		pk_bitfield_contain (transaction->priv->cached_transaction_flags,
				     PK_TRANSACTION_FLAG_ENUM_ONLY_TRUSTED));

	/* set transaction state */
	pk_transaction_set_state (transaction,
				  PK_TRANSACTION_STATE_WAITING_FOR_AUTH);

	/* check subject */
	priv->waiting_for_auth = TRUE;

	/* insert details about the authorization */
	details = polkit_details_new ();

	/* do we have package details? */
	if (priv->cached_package_id != NULL)
		package_ids = g_strdup (priv->cached_package_id);
	else if (priv->cached_package_ids != NULL)
		package_ids = pk_package_ids_to_string (priv->cached_package_ids);

	/* save optional stuff */
	if (package_ids != NULL)
		polkit_details_insert (details, "package_ids", package_ids);
	if (priv->cmdline != NULL)
		polkit_details_insert (details, "cmdline", priv->cmdline);

	/* do not use the default icon and wording for some roles */
	if (role == PK_ROLE_ENUM_INSTALL_PACKAGES &&
	    role == PK_ROLE_ENUM_UPDATE_PACKAGES &&
	    !pk_bitfield_contain (priv->cached_transaction_flags, PK_TRANSACTION_FLAG_ENUM_ONLY_TRUSTED)) {

		/* don't use the friendly PackageKit icon as this is
		 * might be a ricky authorisation */
		polkit_details_insert (details, "polkit.icon_name", "emblem-important");

		string = g_string_new ("");

		/* TRANSLATORS: is not GPG signed */
		g_string_append (string, g_dgettext (GETTEXT_PACKAGE, N_("The software is not from a trusted source.")));
		g_string_append (string, "\n");

		/* UpdatePackages */
		if (priv->role == PK_ROLE_ENUM_UPDATE_PACKAGES) {

			/* TRANSLATORS: user has to trust provider -- I know, this sucks */
			text = g_dngettext (GETTEXT_PACKAGE,
					    N_("Do not update this package unless you are sure it is safe to do so."),
					    N_("Do not update these packages unless you are sure it is safe to do so."),
					    g_strv_length (priv->cached_package_ids));
			g_string_append (string, text);
		}

		/* InstallPackages */
		if (priv->role == PK_ROLE_ENUM_INSTALL_PACKAGES) {

			/* TRANSLATORS: user has to trust provider -- I know, this sucks */
			text = g_dngettext (GETTEXT_PACKAGE,
					    N_("Do not install this package unless you are sure it is safe to do so."),
					    N_("Do not install these packages unless you are sure it is safe to do so."),
					    g_strv_length (priv->cached_package_ids));
			g_string_append (string, text);
		}
		if (string->len > 0) {
			polkit_details_insert (details, "polkit.gettext_domain", GETTEXT_PACKAGE);
			polkit_details_insert (details, "polkit.message", string->str);
		}
	}

	/* do authorization async */
	polkit_authority_check_authorization (priv->authority,
					      priv->subject,
					      action_id,
					      details,
					      POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION,
					      priv->cancellable,
					      (GAsyncReadyCallback) pk_transaction_action_obtain_authorization_finished_cb,
					      transaction);
	return TRUE;
}