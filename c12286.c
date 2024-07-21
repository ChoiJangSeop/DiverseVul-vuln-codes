ngx_http_lua_copy_request_headers(ngx_http_request_t *sr, ngx_http_request_t *r)
{
    ngx_table_elt_t                 *header;
    ngx_list_part_t                 *part;
    ngx_uint_t                       i;

    if (ngx_list_init(&sr->headers_in.headers, sr->pool, 20,
                      sizeof(ngx_table_elt_t)) != NGX_OK)
    {
        return NGX_ERROR;
    }

    dd("before: parent req headers count: %d",
       (int) r->headers_in.headers.part.nelts);

    part = &r->headers_in.headers.part;
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

        dd("setting request header %.*s: %.*s", (int) header[i].key.len,
           header[i].key.data, (int) header[i].value.len,
           header[i].value.data);

        if (ngx_http_lua_set_input_header(sr, header[i].key,
                                          header[i].value, 0) == NGX_ERROR)
        {
            return NGX_ERROR;
        }
    }

    dd("after: parent req headers count: %d",
       (int) r->headers_in.headers.part.nelts);

    return NGX_OK;
}