ngx_mail_core_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_mail_core_srv_conf_t *prev = parent;
    ngx_mail_core_srv_conf_t *conf = child;

    ngx_conf_merge_msec_value(conf->timeout, prev->timeout, 60000);
    ngx_conf_merge_msec_value(conf->resolver_timeout, prev->resolver_timeout,
                              30000);


    ngx_conf_merge_str_value(conf->server_name, prev->server_name, "");

    if (conf->server_name.len == 0) {
        conf->server_name = cf->cycle->hostname;
    }

    if (conf->protocol == NULL) {
        ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                      "unknown mail protocol for server in %s:%ui",
                      conf->file_name, conf->line);
        return NGX_CONF_ERROR;
    }

    if (conf->error_log == NULL) {
        if (prev->error_log) {
            conf->error_log = prev->error_log;
        } else {
            conf->error_log = &cf->cycle->new_log;
        }
    }

    ngx_conf_merge_ptr_value(conf->resolver, prev->resolver, NULL);

    return NGX_CONF_OK;
}