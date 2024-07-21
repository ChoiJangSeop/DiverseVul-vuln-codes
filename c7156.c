ews_backend_constructed (GObject *object)
{
	EBackend *backend;
	ESource *source;
	ESourceAuthentication *auth_extension;
	const gchar *extension_name;
	gchar *host = NULL;
	guint16 port = 0;

	/* Chain up to parent's constructed() method. */
	G_OBJECT_CLASS (e_ews_backend_parent_class)->constructed (object);

	backend = E_BACKEND (object);
	source = e_backend_get_source (backend);

	/* XXX Wondering if we ought to delay this until after folders
	 *     are initially populated, just to remove the possibility
	 *     of weird races with clients trying to create folders. */
	e_server_side_source_set_remote_creatable (
		E_SERVER_SIDE_SOURCE (source), TRUE);

	/* Setup the Authentication extension so
	 * Camel can determine host reachability. */
	extension_name = E_SOURCE_EXTENSION_AUTHENTICATION;
	auth_extension = e_source_get_extension (source, extension_name);

	if (e_backend_get_destination_address (backend, &host, &port)) {
		e_source_authentication_set_host (auth_extension, host);
		e_source_authentication_set_port (auth_extension, port);
	}

	g_free (host);

	/* Reset the connectable, it steals data from Authentication extension,
	   where is written incorrect address */
	e_backend_set_connectable (backend, NULL);
}