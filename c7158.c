ews_backend_authenticate_sync (EBackend *backend,
			       const ENamedParameters *credentials,
			       gchar **out_certificate_pem,
			       GTlsCertificateFlags *out_certificate_errors,
			       GCancellable *cancellable,
			       GError **error)
{
	EEwsBackend *ews_backend;
	EEwsConnection *connection;
	CamelEwsSettings *ews_settings;
	ESourceAuthenticationResult result = E_SOURCE_AUTHENTICATION_ERROR;

	g_return_val_if_fail (E_IS_EWS_BACKEND (backend), E_SOURCE_AUTHENTICATION_ERROR);

	ews_backend = E_EWS_BACKEND (backend);
	ews_settings = ews_backend_get_settings (ews_backend);
	g_return_val_if_fail (ews_settings != NULL, E_SOURCE_AUTHENTICATION_ERROR);

	g_mutex_lock (&ews_backend->priv->connection_lock);
	g_clear_object (&ews_backend->priv->connection);
	e_named_parameters_free (ews_backend->priv->credentials);
	ews_backend->priv->credentials = e_named_parameters_new_clone (credentials);
	g_mutex_unlock (&ews_backend->priv->connection_lock);

	connection = e_ews_backend_ref_connection_sync (ews_backend, &result, cancellable, error);
	g_clear_object (&connection);

	if (result == E_SOURCE_AUTHENTICATION_ACCEPTED) {
		e_collection_backend_authenticate_children (E_COLLECTION_BACKEND (backend), credentials);

		e_ews_backend_sync_folders (ews_backend, NULL, ews_backend_folders_synced_cb, NULL);
	} else if (e_ews_connection_utils_get_without_password (ews_settings) &&
		   result == E_SOURCE_AUTHENTICATION_REJECTED &&
		   !e_named_parameters_exists (credentials, E_SOURCE_CREDENTIAL_PASSWORD)) {
		e_ews_connection_utils_force_off_ntlm_auth_check ();
		result = E_SOURCE_AUTHENTICATION_REQUIRED;
	}

	return result;
}