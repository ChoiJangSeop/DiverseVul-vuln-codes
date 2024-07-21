static int process_http_upload(
                struct MHD_Connection *connection,
                const char *upload_data,
                size_t *upload_data_size,
                RemoteSource *source) {

        bool finished = false;
        size_t remaining;
        int r;

        assert(source);

        log_trace("%s: connection %p, %zu bytes",
                  __func__, connection, *upload_data_size);

        if (*upload_data_size) {
                log_trace("Received %zu bytes", *upload_data_size);

                r = journal_importer_push_data(&source->importer,
                                               upload_data, *upload_data_size);
                if (r < 0)
                        return mhd_respond_oom(connection);

                *upload_data_size = 0;
        } else
                finished = true;

        for (;;) {
                r = process_source(source,
                                   journal_remote_server_global->compress,
                                   journal_remote_server_global->seal);
                if (r == -EAGAIN)
                        break;
                if (r < 0) {
                        if (r == -E2BIG)
                                log_warning_errno(r, "Entry is too above maximum of %u, aborting connection %p.",
                                                  DATA_SIZE_MAX, connection);
                        else
                                log_warning_errno(r, "Failed to process data, aborting connection %p: %m",
                                                  connection);
                        return MHD_NO;
                }
        }

        if (!finished)
                return MHD_YES;

        /* The upload is finished */

        remaining = journal_importer_bytes_remaining(&source->importer);
        if (remaining > 0) {
                log_warning("Premature EOF byte. %zu bytes lost.", remaining);
                return mhd_respondf(connection,
                                    0, MHD_HTTP_EXPECTATION_FAILED,
                                    "Premature EOF. %zu bytes of trailing data not processed.",
                                    remaining);
        }

        return mhd_respond(connection, MHD_HTTP_ACCEPTED, "OK.");
};