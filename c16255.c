rsvg_handle_set_base_uri (RsvgHandle * handle, const char *base_uri)
{
    gchar *uri;

    g_return_if_fail (handle != NULL);

    if (base_uri == NULL)
	return;

    if (rsvg_path_is_uri (base_uri)) 
        uri = g_strdup (base_uri);
    else
        uri = rsvg_get_base_uri_from_filename (base_uri);

    if (uri) {
        if (handle->priv->base_uri)
            g_free (handle->priv->base_uri);
        handle->priv->base_uri = uri;
    }
}