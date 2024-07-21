int meth_post(struct transaction_t *txn, void *params)
{
    struct meth_params *pparams = (struct meth_params *) params;
    struct strlist *action;
    int r, ret;
    size_t len;

    /* Response should not be cached */
    txn->flags.cc |= CC_NOCACHE;

    /* Parse the path */
    r = dav_parse_req_target(txn, pparams);
    if (r) return r;

    /* Make sure method is allowed (only allowed on certain collections) */
    if (!(txn->req_tgt.allow & ALLOW_POST)) return HTTP_NOT_ALLOWED;

    /* Do any special processing */
    if (pparams->post.proc) {
        ret = pparams->post.proc(txn);
        if (ret != HTTP_CONTINUE) return ret;
    }

    /* Check for query params */
    action = hash_lookup("action", &txn->req_qparams);

    if (!action) {
        /* Check Content-Type */
        const char **hdr = spool_getheader(txn->req_hdrs, "Content-Type");

        if ((pparams->post.allowed & POST_SHARE) && hdr &&
            (is_mediatype(hdr[0], DAVSHARING_CONTENT_TYPE) ||
             is_mediatype(hdr[0], "text/xml"))) {
            /* Sharing request */
            return dav_post_share(txn, pparams);
        }
        else if (pparams->post.bulk.data_prop && hdr &&
                 is_mediatype(hdr[0], "application/xml")) {
            /* Bulk CRUD */
            return HTTP_FORBIDDEN;
        }
        else if (pparams->post.bulk.import && hdr) {
            /* Bulk import */
            return dav_post_import(txn, pparams);
        }
        else return HTTP_BAD_REQUEST;
    }

    if (!(pparams->post.allowed & POST_ADDMEMBER) ||
        !action || action->next || strcmp(action->s, "add-member")) {
        return HTTP_BAD_REQUEST;
    }

    /* POST add-member to regular collection */

    /* Append a unique resource name to URL path and perform a PUT */
    len = strlen(txn->req_tgt.path);
    txn->req_tgt.resource = txn->req_tgt.path + len;
    txn->req_tgt.reslen =
        snprintf(txn->req_tgt.resource, MAX_MAILBOX_PATH - len,
                 "%s.%s", makeuuid(), pparams->mime_types[0].file_ext ?
                 pparams->mime_types[0].file_ext : "");

    /* Tell client where to find the new resource */
    txn->location = txn->req_tgt.path;

    ret = meth_put(txn, params);

    if (ret != HTTP_CREATED) txn->location = NULL;

    return ret;
}