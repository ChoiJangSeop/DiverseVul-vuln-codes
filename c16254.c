rsvg_acquire_data_data (const char *uri,
                        const char *base_uri, 
                        char **out_mime_type,
                        gsize *out_len,
                        GError **error)
{
    const char *comma, *start, *end;
    char *mime_type;
    char *data;
    gsize data_len;
    gboolean base64 = FALSE;

    g_assert (out_len != NULL);
    g_assert (g_str_has_prefix (uri, "data:"));

    mime_type = NULL;
    start = uri + 5;
    comma = strchr (start, ',');

    if (comma && comma != start) {
        /* Deal with MIME type / params */
        if (comma > start + BASE64_INDICATOR_LEN && 
            !g_ascii_strncasecmp (comma - BASE64_INDICATOR_LEN, BASE64_INDICATOR, BASE64_INDICATOR_LEN)) {
            end = comma - BASE64_INDICATOR_LEN;
            base64 = TRUE;
        } else {
            end = comma;
        }

        if (end != start) {
            mime_type = uri_decoded_copy (start, end - start);
        }
    }

    if (comma)
        start = comma + 1;

    if (*start) {
        data = uri_decoded_copy (start, strlen (start));

        if (base64)
            data = g_base64_decode_inplace ((char*) data, &data_len);
        else
            data_len = strlen ((const char *) data);
    } else {
        data = NULL;
        data_len = 0;
    }

    if (out_mime_type)
        *out_mime_type = mime_type;
    else
        g_free (mime_type);

    *out_len = data_len;
    return data;
}