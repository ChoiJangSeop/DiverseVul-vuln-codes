static int h2_task_pre_conn(conn_rec* c, void *arg)
{
    h2_ctx *ctx;
    
    if (!c->master) {
        return OK;
    }
    
    ctx = h2_ctx_get(c, 0);
    (void)arg;
    if (ctx->task) {
        ap_log_cerror(APLOG_MARK, APLOG_TRACE2, 0, c,
                      "h2_h2, pre_connection, found stream task");
        ap_add_input_filter("H2_SLAVE_IN", NULL, NULL, c);
        ap_add_output_filter("H2_PARSE_H1", NULL, NULL, c);
        ap_add_output_filter("H2_SLAVE_OUT", NULL, NULL, c);
    }
    return OK;
}