int meth_get_head(struct transaction_t *txn, void *params)
{
    struct meth_params *gparams = (struct meth_params *) params;
    const char **hdr;
    struct mime_type_t *mime = NULL;
    int ret = 0, r = 0, precond, rights;
    const char *data = NULL;
    unsigned long datalen = 0, offset = 0;
    struct buf msg_buf = BUF_INITIALIZER;
    struct resp_body_t *resp_body = &txn->resp_body;
    struct mailbox *mailbox = NULL;
    struct dav_data *ddata;
    struct index_record record;
    const char *etag = NULL;
    time_t lastmod = 0;
    void *davdb = NULL, *obj = NULL;
    char *freeme = NULL;

    /* Parse the path */
    r = dav_parse_req_target(txn, gparams);
    if (r) return r;

    if (txn->req_tgt.namespace->id == URL_NS_PRINCIPAL) {
        /* Special "principal" */
        if (txn->req_tgt.flags == TGT_SERVER_INFO) return get_server_info(txn);

        /* No content for principals (yet) */
        return HTTP_NO_CONTENT;
    }

    if (!txn->req_tgt.resource) {
        /* Do any collection processing */
        if (gparams->get) return gparams->get(txn, NULL, NULL, NULL, NULL);

        /* We don't handle GET on a collection */
        return HTTP_NO_CONTENT;
    }

    /* Check ACL for current user */
    rights = httpd_myrights(httpd_authstate, txn->req_tgt.mbentry);
    if ((rights & DACL_READ) != DACL_READ) {
        /* DAV:need-privileges */
        txn->error.precond = DAV_NEED_PRIVS;
        txn->error.resource = txn->req_tgt.path;
        txn->error.rights = DACL_READ;
        return HTTP_NO_PRIVS;
    }

    if (gparams->mime_types) {
        /* Check requested MIME type:
           1st entry in gparams->mime_types array MUST be default MIME type */
        if ((hdr = spool_getheader(txn->req_hdrs, "Accept")))
            mime = get_accept_type(hdr, gparams->mime_types);
        else mime = gparams->mime_types;
        if (!mime) return HTTP_NOT_ACCEPTABLE;
    }

    if (txn->req_tgt.mbentry->server) {
        /* Remote mailbox */
        struct backend *be;

        be = proxy_findserver(txn->req_tgt.mbentry->server,
                              &http_protocol, httpd_userid,
                              &backend_cached, NULL, NULL, httpd_in);
        if (!be) return HTTP_UNAVAILABLE;

        return http_pipe_req_resp(be, txn);
    }

    /* Local Mailbox */

    /* Open mailbox for reading */
    r = mailbox_open_irl(txn->req_tgt.mbentry->name, &mailbox);
    if (r) {
        syslog(LOG_ERR, "http_mailbox_open(%s) failed: %s",
               txn->req_tgt.mbentry->name, error_message(r));
        goto done;
    }

    /* Open the DAV DB corresponding to the mailbox */
    davdb = gparams->davdb.open_db(mailbox);

    /* Find message UID for the resource */
    gparams->davdb.lookup_resource(davdb, txn->req_tgt.mbentry->name,
                                   txn->req_tgt.resource, (void **) &ddata, 0);
    if (!ddata->rowid) {
        ret = HTTP_NOT_FOUND;
        goto done;
    }

    /* Fetch resource validators */
    r = gparams->get_validators(mailbox, (void *) ddata, httpd_userid,
                                &record, &etag, &lastmod);
    if (r) {
        txn->error.desc = error_message(r);
        ret = HTTP_SERVER_ERROR;
        goto done;
    }

    txn->flags.ranges = (ddata->imap_uid != 0);

    /* Check any preconditions, including range request */
    precond = gparams->check_precond(txn, params, mailbox,
                                     (void *) ddata, etag, lastmod);

    switch (precond) {
    case HTTP_OK:
    case HTTP_PARTIAL:
    case HTTP_NOT_MODIFIED:
        /* Fill in ETag, Last-Modified, Expires, and Cache-Control */
        resp_body->etag = etag;
        resp_body->lastmod = lastmod;
        resp_body->maxage = 3600;       /* 1 hr */
        txn->flags.cc |= CC_MAXAGE | CC_REVALIDATE;  /* don't use stale data */

        if (precond != HTTP_NOT_MODIFIED && record.uid) break;

        GCC_FALLTHROUGH

    default:
        /* We failed a precondition - don't perform the request */
        ret = precond;
        goto done;
    }

    /* Do any special processing */
    if (gparams->get) {
        ret = gparams->get(txn, mailbox, &record, ddata, &obj);
        if (ret != HTTP_CONTINUE) goto done;

        ret = 0;
    }

    if (mime && !resp_body->type) {
        txn->flags.vary |= VARY_ACCEPT;
        resp_body->type = mime->content_type;
    }

    if (!obj) {
        /* Raw resource - length doesn't include RFC 5322 header */
        offset = record.header_size;
        datalen = record.size - offset;

        if (txn->meth == METH_GET) {
            /* Load message containing the resource */
            r = mailbox_map_record(mailbox, &record, &msg_buf);
            if (r) goto done;

            data = buf_base(&msg_buf) + offset;

            if (mime != gparams->mime_types) {
                /* Not the storage format - create resource object */
                struct buf inbuf;

                buf_init_ro(&inbuf, data, datalen);
                obj = gparams->mime_types[0].to_object(&inbuf);
                buf_free(&inbuf);
            }
        }
    }

    if (obj) {
        /* Convert object into requested MIME type */
        struct buf *outbuf = mime->from_object(obj);

        datalen = buf_len(outbuf);
        if (txn->meth == METH_GET) data = freeme = buf_release(outbuf);
        buf_destroy(outbuf);

        if (gparams->mime_types[0].free) gparams->mime_types[0].free(obj);
    }

    write_body(precond, txn, data, datalen);

    buf_free(&msg_buf);
    free(freeme);

  done:
    if (davdb) gparams->davdb.close_db(davdb);
    if (r) {
        txn->error.desc = error_message(r);
        ret = HTTP_SERVER_ERROR;
    }
    mailbox_close(&mailbox);

    return ret;
}