_bluetooth_client_set_discoverable (BluetoothClient *client,
				    gboolean discoverable,
				    guint timeout)
{
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE (client);
	GtkTreePath *path;
	GObject *adapter;
	GtkTreeIter iter;

	g_return_val_if_fail (BLUETOOTH_IS_CLIENT (client), FALSE);

	if (priv->default_adapter == NULL)
		return FALSE;

	path = gtk_tree_row_reference_get_path (priv->default_adapter);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->store), &iter, path);
	gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
                            BLUETOOTH_COLUMN_PROXY, &adapter, -1);
        gtk_tree_path_free (path);

	if (adapter == NULL)
		return FALSE;

	g_object_set (adapter,
		      "discoverable", discoverable,
		      "discoverable-timeout", timeout,
		      NULL);
	g_object_unref (adapter);

	return TRUE;
}