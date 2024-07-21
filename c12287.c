ngx_http_lua_adjust_subrequest(ngx_http_request_t *sr, ngx_uint_t method,
    int always_forward_body, ngx_http_request_body_t *body,
    unsigned vars_action, ngx_array_t *extra_vars)
{
    ngx_http_request_t          *r;
    ngx_int_t                    rc;
    ngx_http_core_main_conf_t   *cmcf;
    size_t                       size;

    r = sr->parent;

    sr->header_in = r->header_in;

    if (body) {
        sr->request_body = body;

        rc = ngx_http_lua_set_content_length_header(sr,
                                                    body->buf
                                                    ? ngx_buf_size(body->buf)
                                                    : 0);

        if (rc != NGX_OK) {
            return NGX_ERROR;
        }

    } else if (!always_forward_body
               && method != NGX_HTTP_PUT
               && method != NGX_HTTP_POST
               && r->headers_in.content_length_n > 0)
    {
        rc = ngx_http_lua_set_content_length_header(sr, 0);
        if (rc != NGX_OK) {
            return NGX_ERROR;
        }

#if 1
        sr->request_body = NULL;
#endif

    } else {
        if (ngx_http_lua_copy_request_headers(sr, r) != NGX_OK) {
            return NGX_ERROR;
        }

        if (sr->request_body) {

            /* deep-copy the request body */

            if (sr->request_body->temp_file) {
                if (ngx_http_lua_copy_in_file_request_body(sr) != NGX_OK) {
                    return NGX_ERROR;
                }
            }
        }
    }

    sr->method = method;

    switch (method) {
        case NGX_HTTP_GET:
            sr->method_name = ngx_http_lua_get_method;
            break;

        case NGX_HTTP_POST:
            sr->method_name = ngx_http_lua_post_method;
            break;

        case NGX_HTTP_PUT:
            sr->method_name = ngx_http_lua_put_method;
            break;

        case NGX_HTTP_HEAD:
            sr->method_name = ngx_http_lua_head_method;
            break;

        case NGX_HTTP_DELETE:
            sr->method_name = ngx_http_lua_delete_method;
            break;

        case NGX_HTTP_OPTIONS:
            sr->method_name = ngx_http_lua_options_method;
            break;

        case NGX_HTTP_MKCOL:
            sr->method_name = ngx_http_lua_mkcol_method;
            break;

        case NGX_HTTP_COPY:
            sr->method_name = ngx_http_lua_copy_method;
            break;

        case NGX_HTTP_MOVE:
            sr->method_name = ngx_http_lua_move_method;
            break;

        case NGX_HTTP_PROPFIND:
            sr->method_name = ngx_http_lua_propfind_method;
            break;

        case NGX_HTTP_PROPPATCH:
            sr->method_name = ngx_http_lua_proppatch_method;
            break;

        case NGX_HTTP_LOCK:
            sr->method_name = ngx_http_lua_lock_method;
            break;

        case NGX_HTTP_UNLOCK:
            sr->method_name = ngx_http_lua_unlock_method;
            break;

        case NGX_HTTP_PATCH:
            sr->method_name = ngx_http_lua_patch_method;
            break;

        case NGX_HTTP_TRACE:
            sr->method_name = ngx_http_lua_trace_method;
            break;

        default:
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "unsupported HTTP method: %u", (unsigned) method);

            return NGX_ERROR;
    }

    if (!(vars_action & NGX_HTTP_LUA_SHARE_ALL_VARS)) {
        /* we do not inherit the parent request's variables */
        cmcf = ngx_http_get_module_main_conf(sr, ngx_http_core_module);

        size = cmcf->variables.nelts * sizeof(ngx_http_variable_value_t);

        if (vars_action & NGX_HTTP_LUA_COPY_ALL_VARS) {

            sr->variables = ngx_palloc(sr->pool, size);
            if (sr->variables == NULL) {
                return NGX_ERROR;
            }

            ngx_memcpy(sr->variables, r->variables, size);

        } else {

            /* we do not inherit the parent request's variables */

            sr->variables = ngx_pcalloc(sr->pool, size);
            if (sr->variables == NULL) {
                return NGX_ERROR;
            }
        }
    }

    return ngx_http_lua_subrequest_add_extra_vars(sr, extra_vars);
}