ngx_http_lua_set_content_length_header(ngx_http_request_t *r, off_t len)
{
    ngx_table_elt_t                 *h, *header;
    u_char                          *p;
    ngx_list_part_t                 *part;
    ngx_http_request_t              *pr;
    ngx_uint_t                       i;

    r->headers_in.content_length_n = len;

    if (ngx_list_init(&r->headers_in.headers, r->pool, 20,
                      sizeof(ngx_table_elt_t)) != NGX_OK)
    {
        return NGX_ERROR;
    }

    h = ngx_list_push(&r->headers_in.headers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    h->key = ngx_http_lua_content_length_header_key;
    h->lowcase_key = ngx_pnalloc(r->pool, h->key.len);
    if (h->lowcase_key == NULL) {
        return NGX_ERROR;
    }

    ngx_strlow(h->lowcase_key, h->key.data, h->key.len);

    r->headers_in.content_length = h;

    p = ngx_palloc(r->pool, NGX_OFF_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    h->value.data = p;

    h->value.len = ngx_sprintf(h->value.data, "%O", len) - h->value.data;

    h->hash = ngx_http_lua_content_length_hash;

#if 0
    dd("content length hash: %lu == %lu", (unsigned long) h->hash,
       ngx_hash_key_lc((u_char *) "Content-Length",
                       sizeof("Content-Length") - 1));
#endif

    dd("r content length: %.*s",
       (int) r->headers_in.content_length->value.len,
       r->headers_in.content_length->value.data);

    pr = r->parent;

    if (pr == NULL) {
        return NGX_OK;
    }

    /* forward the parent request's all other request headers */

    part = &pr->headers_in.headers.part;
    header = part->elts;

    for (i = 0; /* void */; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            header = part->elts;
            i = 0;
        }

        if (header[i].key.len == sizeof("Content-Length") - 1
            && ngx_strncasecmp(header[i].key.data, (u_char *) "Content-Length",
                               sizeof("Content-Length") - 1) == 0)
        {
            continue;
        }

        if (ngx_http_lua_set_input_header(r, header[i].key,
                                          header[i].value, 0) == NGX_ERROR)
        {
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}