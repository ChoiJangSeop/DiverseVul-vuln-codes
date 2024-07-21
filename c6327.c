create_file(ngx_cycle_t *cycle, const u_char *filename, const u_char *contents, size_t len) {
    FILE  *f;
    int    ret;
    size_t total_written = 0, written;

    f = fopen((const char *) filename, "w");
    if (f != NULL) {
        /* We must do something with these return values because
         * otherwise on some platforms it will cause a compiler
         * warning.
         */
        do {
            ret = fchmod(fileno(f), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        } while (ret == -1 && errno == EINTR);
        do {
            written = fwrite(contents + total_written, 1,
                len - total_written, f);
            total_written += written;
        } while (total_written < len);
        fclose(f);
        return NGX_OK;
    } else {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
            "could not create %s", filename);
            return NGX_ERROR;
    }
}