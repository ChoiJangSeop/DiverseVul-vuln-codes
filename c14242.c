ngx_mail_read_command(ngx_mail_session_t *s, ngx_connection_t *c)
{
    ssize_t                    n;
    ngx_int_t                  rc;
    ngx_str_t                  l;
    ngx_mail_core_srv_conf_t  *cscf;

    if (s->buffer->last < s->buffer->end) {

        n = c->recv(c, s->buffer->last, s->buffer->end - s->buffer->last);

        if (n == NGX_ERROR || n == 0) {
            ngx_mail_close_connection(c);
            return NGX_ERROR;
        }

        if (n > 0) {
            s->buffer->last += n;
        }

        if (n == NGX_AGAIN) {
            if (s->buffer->pos == s->buffer->last) {
                return NGX_AGAIN;
            }
        }
    }

    cscf = ngx_mail_get_module_srv_conf(s, ngx_mail_core_module);

    rc = cscf->protocol->parse_command(s);

    if (rc == NGX_AGAIN) {

        if (s->buffer->last < s->buffer->end) {
            return rc;
        }

        l.len = s->buffer->last - s->buffer->start;
        l.data = s->buffer->start;

        ngx_log_error(NGX_LOG_INFO, c->log, 0,
                      "client sent too long command \"%V\"", &l);

        s->quit = 1;

        return NGX_MAIL_PARSE_INVALID_COMMAND;
    }

    if (rc == NGX_IMAP_NEXT || rc == NGX_MAIL_PARSE_INVALID_COMMAND) {
        return rc;
    }

    if (rc == NGX_ERROR) {
        ngx_mail_close_connection(c);
        return NGX_ERROR;
    }

    return NGX_OK;
}