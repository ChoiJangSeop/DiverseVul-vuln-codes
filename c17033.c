test_non_svg_element (void)
{
    char *filename = get_test_filename ("335-non-svg-element.svg");
    RsvgHandle *handle;
    GError *error = NULL;

    handle = rsvg_handle_new_from_file (filename, &error);
    g_free (filename);

    g_assert (handle == NULL);
    g_assert (g_error_matches (error, RSVG_ERROR, RSVG_ERROR_FAILED));

    g_error_free (error);
}