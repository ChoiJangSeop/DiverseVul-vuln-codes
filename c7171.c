e_ews_connection_utils_prepare_message (EEwsConnection *cnc,
					SoupMessage *message,
					GCancellable *cancellable)
{
	ESoupAuthBearer *using_bearer_auth;
	GError *local_error = NULL;

	if (!ews_connection_utils_maybe_prepare_bearer_auth (cnc, message, cancellable))
		return FALSE;

	using_bearer_auth = e_ews_connection_ref_bearer_auth (cnc);

	if (using_bearer_auth &&
	    e_soup_auth_bearer_is_expired (using_bearer_auth) &&
	    !ews_connection_utils_setup_bearer_auth (cnc, message, FALSE, using_bearer_auth, cancellable, &local_error)) {
		if (local_error) {
			soup_message_set_status_full (message, SOUP_STATUS_BAD_REQUEST, local_error->message);
			g_clear_error (&local_error);
		} else {
			soup_message_set_status (message, SOUP_STATUS_BAD_REQUEST);
		}

		g_object_unref (using_bearer_auth);

		return FALSE;
	}

	g_clear_object (&using_bearer_auth);

	return TRUE;
}