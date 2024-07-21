conn_rec *h2_slave_create(conn_rec *master, int slave_id, apr_pool_t *parent)
{
    apr_allocator_t *allocator;
    apr_status_t status;
    apr_pool_t *pool;
    conn_rec *c;
    void *cfg;
    module *mpm;
    
    ap_assert(master);
    ap_log_cerror(APLOG_MARK, APLOG_TRACE3, 0, master,
                  "h2_stream(%ld-%d): create slave", master->id, slave_id);
    
    /* We create a pool with its own allocator to be used for
     * processing a request. This is the only way to have the processing
     * independant of its parent pool in the sense that it can work in
     * another thread. Also, the new allocator needs its own mutex to
     * synchronize sub-pools.
     */
    apr_allocator_create(&allocator);
    apr_allocator_max_free_set(allocator, ap_max_mem_free);
    status = apr_pool_create_ex(&pool, parent, NULL, allocator);
    if (status != APR_SUCCESS) {
        ap_log_cerror(APLOG_MARK, APLOG_ERR, status, master, 
                      APLOGNO(10004) "h2_session(%ld-%d): create slave pool",
                      master->id, slave_id);
        return NULL;
    }
    apr_allocator_owner_set(allocator, pool);
    apr_pool_abort_set(abort_on_oom, pool);
    apr_pool_tag(pool, "h2_slave_conn");

    c = (conn_rec *) apr_palloc(pool, sizeof(conn_rec));
    if (c == NULL) {
        ap_log_cerror(APLOG_MARK, APLOG_ERR, APR_ENOMEM, master, 
                      APLOGNO(02913) "h2_session(%ld-%d): create slave",
                      master->id, slave_id);
        apr_pool_destroy(pool);
        return NULL;
    }
    
    memcpy(c, master, sizeof(conn_rec));
        
    c->master                 = master;
    c->pool                   = pool;   
    c->conn_config            = ap_create_conn_config(pool);
    c->notes                  = apr_table_make(pool, 5);
    c->input_filters          = NULL;
    c->output_filters         = NULL;
#if AP_MODULE_MAGIC_AT_LEAST(20180903, 1)
    c->filter_conn_ctx        = NULL;
#endif
    c->bucket_alloc           = apr_bucket_alloc_create(pool);
#if !AP_MODULE_MAGIC_AT_LEAST(20180720, 1)
    c->data_in_input_filters  = 0;
    c->data_in_output_filters = 0;
#endif
    /* prevent mpm_event from making wrong assumptions about this connection,
     * like e.g. using its socket for an async read check. */
    c->clogging_input_filters = 1;
    c->log                    = NULL;
    c->log_id                 = apr_psprintf(pool, "%ld-%d", 
                                             master->id, slave_id);
    c->aborted                = 0;
    /* We cannot install the master connection socket on the slaves, as
     * modules mess with timeouts/blocking of the socket, with
     * unwanted side effects to the master connection processing.
     * Fortunately, since we never use the slave socket, we can just install
     * a single, process-wide dummy and everyone is happy.
     */
    ap_set_module_config(c->conn_config, &core_module, dummy_socket);
    /* TODO: these should be unique to this thread */
    c->sbh                    = master->sbh;
    /* TODO: not all mpm modules have learned about slave connections yet.
     * copy their config from master to slave.
     */
    if ((mpm = h2_conn_mpm_module()) != NULL) {
        cfg = ap_get_module_config(master->conn_config, mpm);
        ap_set_module_config(c->conn_config, mpm, cfg);
    }

    ap_log_cerror(APLOG_MARK, APLOG_TRACE2, 0, c, 
                  "h2_stream(%ld-%d): created slave", master->id, slave_id);
    return c;
}