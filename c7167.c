ews_connection_finalize (GObject *object)
{
	EEwsConnectionPrivate *priv;

	priv = E_EWS_CONNECTION_GET_PRIVATE (object);

	g_free (priv->uri);
	g_free (priv->password);
	g_free (priv->email);
	g_free (priv->hash_key);
	g_free (priv->impersonate_user);

	g_clear_object (&priv->bearer_auth);

	g_mutex_clear (&priv->property_lock);
	g_rec_mutex_clear (&priv->queue_lock);
	g_mutex_clear (&priv->notification_lock);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (e_ews_connection_parent_class)->finalize (object);
}