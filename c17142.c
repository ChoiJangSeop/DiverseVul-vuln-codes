static void check_serverpath(request_rec *r)
{
    server_rec *s;
    server_rec *last_s;
    name_chain *src;
    apr_port_t port;

    port = r->connection->local_addr->port;

    /*
     * This is in conjunction with the ServerPath code in http_core, so we
     * get the right host attached to a non- Host-sending request.
     *
     * See the comment in check_hostalias about how each vhost can be
     * listed multiple times.
     */

    last_s = NULL;
    for (src = r->connection->vhost_lookup_data; src; src = src->next) {
        /* We only consider addresses on the name_chain which have a matching
         * port
         */
        if (src->sar->host_port != 0 && port != src->sar->host_port) {
            continue;
        }

        s = src->server;
        if (s == last_s) {
            continue;
        }
        last_s = s;

        if (s->path && !strncmp(r->uri, s->path, s->pathlen) &&
            (s->path[s->pathlen - 1] == '/' ||
             r->uri[s->pathlen] == '/' ||
             r->uri[s->pathlen] == '\0')) {
            r->server = s;
            return;
        }
    }
}