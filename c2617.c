static int perform_http_xact(void)
{
    /* use free instead of g_free so that we can use xstr* functions from
     * libreport/lib/xfuncs.c
     */
    GHashTable *problem_info = g_hash_table_new_full(g_str_hash, g_str_equal,
                                     free, free);
    /* Read header */
    char *body_start = NULL;
    char *messagebuf_data = NULL;
    unsigned messagebuf_len = 0;
    /* Loop until EOF/error/timeout/end_of_header */
    while (1)
    {
        messagebuf_data = xrealloc(messagebuf_data, messagebuf_len + INPUT_BUFFER_SIZE);
        char *p = messagebuf_data + messagebuf_len;
        int rd = read(STDIN_FILENO, p, INPUT_BUFFER_SIZE);
        if (rd < 0)
        {
            if (errno == EINTR) /* SIGALRM? */
                error_msg_and_die("Timed out");
            perror_msg_and_die("read");
        }
        if (rd == 0)
            break;

        log_debug("Received %u bytes of data", rd);
        messagebuf_len += rd;
        total_bytes_read += rd;
        if (total_bytes_read > MAX_MESSAGE_SIZE)
            error_msg_and_die("Message is too long, aborting");

        /* Check whether we see end of header */
        /* Note: we support both [\r]\n\r\n and \n\n */
        char *past_end = messagebuf_data + messagebuf_len;
        if (p > messagebuf_data+1)
            p -= 2; /* start search from two last bytes in last read - they might be '\n\r' */
        while (p < past_end)
        {
            p = memchr(p, '\n', past_end - p);
            if (!p)
                break;
            p++;
            if (p >= past_end)
                break;
            if (*p == '\n'
             || (*p == '\r' && p+1 < past_end && p[1] == '\n')
            ) {
                body_start = p + 1 + (*p == '\r');
                *p = '\0';
                goto found_end_of_header;
            }
        }
    } /* while (read) */
 found_end_of_header: ;
    log_debug("Request: %s", messagebuf_data);

    /* Sanitize and analyze header.
     * Header now is in messagebuf_data, NUL terminated string,
     * with last empty line deleted (by placement of NUL).
     * \r\n are not (yet) converted to \n, multi-line headers also
     * not converted.
     */
    /* First line must be "op<space>[http://host]/path<space>HTTP/n.n".
     * <space> is exactly one space char.
     */
    if (prefixcmp(messagebuf_data, "DELETE ") == 0)
    {
        messagebuf_data += strlen("DELETE ");
        char *space = strchr(messagebuf_data, ' ');
        if (!space || prefixcmp(space+1, "HTTP/") != 0)
            return 400; /* Bad Request */
        *space = '\0';
        //decode_url(messagebuf_data); %20 => ' '
        alarm(0);
        return delete_path(messagebuf_data);
    }

    /* We erroneously used "PUT /" to create new problems.
     * POST is the correct request in this case:
     * "PUT /" implies creation or replace of resource named "/"!
     * Delete PUT in 2014.
     */
    if (prefixcmp(messagebuf_data, "PUT ") != 0
     && prefixcmp(messagebuf_data, "POST ") != 0
    ) {
        return 400; /* Bad Request */
    }

    enum {
        CREATION_NOTIFICATION,
        CREATION_REQUEST,
    };
    int url_type;
    char *url = skip_non_whitespace(messagebuf_data) + 1; /* skip "POST " */
    if (prefixcmp(url, "/creation_notification ") == 0)
        url_type = CREATION_NOTIFICATION;
    else if (prefixcmp(url, "/ ") == 0)
        url_type = CREATION_REQUEST;
    else
        return 400; /* Bad Request */

    /* Read body */
    if (!body_start)
    {
        log_warning("Premature EOF detected, exiting");
        return 400; /* Bad Request */
    }

    messagebuf_len -= (body_start - messagebuf_data);
    memmove(messagebuf_data, body_start, messagebuf_len);
    log_debug("Body so far: %u bytes, '%s'", messagebuf_len, messagebuf_data);

    /* Loop until EOF/error/timeout */
    while (1)
    {
        if (url_type == CREATION_REQUEST)
        {
            while (1)
            {
                unsigned len = strnlen(messagebuf_data, messagebuf_len);
                if (len >= messagebuf_len)
                    break;
                /* messagebuf has at least one NUL - process the line */
                process_message(problem_info, messagebuf_data);
                messagebuf_len -= (len + 1);
                memmove(messagebuf_data, messagebuf_data + len + 1, messagebuf_len);
            }
        }

        messagebuf_data = xrealloc(messagebuf_data, messagebuf_len + INPUT_BUFFER_SIZE + 1);
        int rd = read(STDIN_FILENO, messagebuf_data + messagebuf_len, INPUT_BUFFER_SIZE);
        if (rd < 0)
        {
            if (errno == EINTR) /* SIGALRM? */
                error_msg_and_die("Timed out");
            perror_msg_and_die("read");
        }
        if (rd == 0)
            break;

        log_debug("Received %u bytes of data", rd);
        messagebuf_len += rd;
        total_bytes_read += rd;
        if (total_bytes_read > MAX_MESSAGE_SIZE)
            error_msg_and_die("Message is too long, aborting");
    }

    /* Body received, EOF was seen. Don't let alarm to interrupt after this. */
    alarm(0);

    if (url_type == CREATION_NOTIFICATION)
    {
        messagebuf_data[messagebuf_len] = '\0';
        return run_post_create(messagebuf_data);
    }

    /* Save problem dir */
    int ret = 0;
    unsigned pid = convert_pid(problem_info);
    die_if_data_is_missing(problem_info);

    char *executable = g_hash_table_lookup(problem_info, FILENAME_EXECUTABLE);
    if (executable)
    {
        char *last_file = concat_path_file(g_settings_dump_location, "last-via-server");
        int repeating_crash = check_recent_crash_file(last_file, executable);
        free(last_file);
        if (repeating_crash) /* Only pretend that we saved it */
            goto out; /* ret is 0: "success" */
    }

#if 0
//TODO:
    /* At least it should generate local problem identifier UUID */
    problem_data_add_basics(problem_info);
//...the problem being that problem_info here is not a problem_data_t!
#endif

    create_problem_dir(problem_info, pid);
    /* does not return */

 out:
    g_hash_table_destroy(problem_info);
    return ret; /* Used as HTTP response code */
}