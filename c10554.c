static void tftp_handle_rrq(Slirp *slirp, struct sockaddr_storage *srcsas,
                            struct tftp_t *tp, int pktlen)
{
    struct tftp_session *spt;
    int s, k;
    size_t prefix_len;
    char *req_fname;
    const char *option_name[2];
    uint32_t option_value[2];
    int nb_options = 0;

    /* check if a session already exists and if so terminate it */
    s = tftp_session_find(slirp, srcsas, tp);
    if (s >= 0) {
        tftp_session_terminate(&slirp->tftp_sessions[s]);
    }

    s = tftp_session_allocate(slirp, srcsas, tp);

    if (s < 0) {
        return;
    }

    spt = &slirp->tftp_sessions[s];

    /* unspecified prefix means service disabled */
    if (!slirp->tftp_prefix) {
        tftp_send_error(spt, 2, "Access violation", tp);
        return;
    }

    /* skip header fields */
    k = 0;
    pktlen -= offsetof(struct tftp_t, x.tp_buf);

    /* prepend tftp_prefix */
    prefix_len = strlen(slirp->tftp_prefix);
    spt->filename = g_malloc(prefix_len + TFTP_FILENAME_MAX + 2);
    memcpy(spt->filename, slirp->tftp_prefix, prefix_len);
    spt->filename[prefix_len] = '/';

    /* get name */
    req_fname = spt->filename + prefix_len + 1;

    while (1) {
        if (k >= TFTP_FILENAME_MAX || k >= pktlen) {
            tftp_send_error(spt, 2, "Access violation", tp);
            return;
        }
        req_fname[k] = tp->x.tp_buf[k];
        if (req_fname[k++] == '\0') {
            break;
        }
    }

    DEBUG_TFTP("tftp rrq file: %s", req_fname);

    /* check mode */
    if ((pktlen - k) < 6) {
        tftp_send_error(spt, 2, "Access violation", tp);
        return;
    }

    if (strcasecmp(&tp->x.tp_buf[k], "octet") != 0) {
        tftp_send_error(spt, 4, "Unsupported transfer mode", tp);
        return;
    }

    k += 6; /* skipping octet */

    /* do sanity checks on the filename */
    if (
#ifdef G_OS_WIN32
        strstr(req_fname, "..\\") ||
        req_fname[strlen(req_fname) - 1] == '\\' ||
#endif
        strstr(req_fname, "../") ||
        req_fname[strlen(req_fname) - 1] == '/') {
        tftp_send_error(spt, 2, "Access violation", tp);
        return;
    }

    /* check if the file exists */
    if (tftp_read_data(spt, 0, NULL, 0) < 0) {
        tftp_send_error(spt, 1, "File not found", tp);
        return;
    }

    if (tp->x.tp_buf[pktlen - 1] != 0) {
        tftp_send_error(spt, 2, "Access violation", tp);
        return;
    }

    while (k < pktlen && nb_options < G_N_ELEMENTS(option_name)) {
        const char *key, *value;

        key = &tp->x.tp_buf[k];
        k += strlen(key) + 1;

        if (k >= pktlen) {
            tftp_send_error(spt, 2, "Access violation", tp);
            return;
        }

        value = &tp->x.tp_buf[k];
        k += strlen(value) + 1;

        if (strcasecmp(key, "tsize") == 0) {
            int tsize = atoi(value);
            struct stat stat_p;

            if (tsize == 0) {
                if (stat(spt->filename, &stat_p) == 0)
                    tsize = stat_p.st_size;
                else {
                    tftp_send_error(spt, 1, "File not found", tp);
                    return;
                }
            }

            option_name[nb_options] = "tsize";
            option_value[nb_options] = tsize;
            nb_options++;
        } else if (strcasecmp(key, "blksize") == 0) {
            int blksize = atoi(value);

            /* Accept blksize up to our maximum size */
            if (blksize > 0) {
                spt->block_size = MIN(blksize, TFTP_BLOCKSIZE_MAX);
                option_name[nb_options] = "blksize";
                option_value[nb_options] = spt->block_size;
                nb_options++;
            }
        }
    }

    if (nb_options > 0) {
        assert(nb_options <= G_N_ELEMENTS(option_name));
        tftp_send_oack(spt, option_name, option_value, nb_options, tp);
        return;
    }

    spt->block_nr = 0;
    tftp_send_next_block(spt, tp);
}