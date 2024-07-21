cr_input_new_from_uri (const gchar * a_file_uri, enum CREncoding a_enc)
{
        CRInput *result = NULL;
        enum CRStatus status = CR_OK;
        FILE *file_ptr = NULL;
        guchar tmp_buf[CR_INPUT_MEM_CHUNK_SIZE] = { 0 };
        gulong nb_read = 0,
                len = 0,
                buf_size = 0;
        gboolean loop = TRUE;
        guchar *buf = NULL;

        g_return_val_if_fail (a_file_uri, NULL);

        file_ptr = fopen (a_file_uri, "r");

        if (file_ptr == NULL) {

#ifdef CR_DEBUG
                cr_utils_trace_debug ("could not open file");
#endif
                g_warning ("Could not open file %s\n", a_file_uri);

                return NULL;
        }

        /*load the file */
        while (loop) {
                nb_read = fread (tmp_buf, 1 /*read bytes */ ,
                                 CR_INPUT_MEM_CHUNK_SIZE /*nb of bytes */ ,
                                 file_ptr);

                if (nb_read != CR_INPUT_MEM_CHUNK_SIZE) {
                        /*we read less chars than we wanted */
                        if (feof (file_ptr)) {
                                /*we reached eof */
                                loop = FALSE;
                        } else {
                                /*a pb occurred !! */
                                cr_utils_trace_debug ("an io error occurred");
                                status = CR_ERROR;
                                goto cleanup;
                        }
                }

                if (status == CR_OK) {
                        /*read went well */
                        buf = g_realloc (buf, len + CR_INPUT_MEM_CHUNK_SIZE);
                        memcpy (buf + len, tmp_buf, nb_read);
                        len += nb_read;
                        buf_size += CR_INPUT_MEM_CHUNK_SIZE;
                }
        }

        if (status == CR_OK) {
                result = cr_input_new_from_buf (buf, len, a_enc, TRUE);
                if (!result) {
                        goto cleanup;
                }
                /*
                 *we should  free buf here because it's own by CRInput.
                 *(see the last parameter of cr_input_new_from_buf().
                 */
                buf = NULL ;
        }

 cleanup:
        if (file_ptr) {
                fclose (file_ptr);
                file_ptr = NULL;
        }

        if (buf) {
                g_free (buf);
                buf = NULL;
        }

        return result;
}