get_preview_thumbnail (const char *uri,
                       int         size)
{
    GdkPixbuf *pixbuf;
    GFile *file;
    GFileInfo *file_info;
    GInputStream *input_stream;
    GObject *object;

    g_return_val_if_fail (uri != NULL, NULL);

    input_stream = NULL;

    file = g_file_new_for_uri (uri);

    /* First see if we can get an input stream via preview::icon  */
    file_info = g_file_query_info (file,
                                   G_FILE_ATTRIBUTE_PREVIEW_ICON,
                                   G_FILE_QUERY_INFO_NONE,
                                   NULL,  /* GCancellable */
                                   NULL); /* return location for GError */
    g_object_unref (file);

    if (file_info == NULL)
      return NULL;

    object = g_object_ref (g_file_info_get_attribute_object (file_info,
                                                             G_FILE_ATTRIBUTE_PREVIEW_ICON));
    g_object_unref (file_info);

    if (!object)
      return NULL;
    if (!G_IS_LOADABLE_ICON (object)) {
      g_object_unref (object);
      return NULL;
    }

    input_stream = g_loadable_icon_load (G_LOADABLE_ICON (object),
                                         0,     /* size */
                                         NULL,  /* return location for type */
                                         NULL,  /* GCancellable */
                                         NULL); /* return location for GError */
    g_object_unref (object);

    if (!input_stream)
      return NULL;

    pixbuf = gdk_pixbuf_new_from_stream_at_scale (input_stream,
                                                  size, size,
                                                  TRUE, NULL, NULL);
    g_object_unref (input_stream);

    return pixbuf;
}