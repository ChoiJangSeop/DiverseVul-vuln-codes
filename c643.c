png_error_callback(png_structp png_read_ptr,
                   png_const_charp error_msg)
{
        LoadContext* lc;
        
        lc = png_get_error_ptr(png_read_ptr);
        
        lc->fatal_error_occurred = TRUE;

        /* I don't trust libpng to call the error callback only once,
         * so check for already-set error
         */
        if (lc->error && *lc->error == NULL) {
                g_set_error (lc->error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                             _("Fatal error reading PNG image file: %s"),
                             error_msg);
        }
}