h2_mplx *h2_mplx_create(conn_rec *c, server_rec *s, apr_pool_t *parent, 
                        h2_workers *workers)
{
    apr_status_t status = APR_SUCCESS;
    apr_allocator_t *allocator;
    apr_thread_mutex_t *mutex;
    h2_mplx *m;
    
    m = apr_pcalloc(parent, sizeof(h2_mplx));
    if (m) {
        m->id = c->id;
        m->c = c;
        m->s = s;
        
        /* We create a pool with its own allocator to be used for
         * processing slave connections. This is the only way to have the
         * processing independant of its parent pool in the sense that it
         * can work in another thread. Also, the new allocator needs its own
         * mutex to synchronize sub-pools.
         */
        status = apr_allocator_create(&allocator);
        if (status != APR_SUCCESS) {
            return NULL;
        }
        apr_allocator_max_free_set(allocator, ap_max_mem_free);
        apr_pool_create_ex(&m->pool, parent, NULL, allocator);
        if (!m->pool) {
            apr_allocator_destroy(allocator);
            return NULL;
        }
        apr_pool_tag(m->pool, "h2_mplx");
        apr_allocator_owner_set(allocator, m->pool);
        status = apr_thread_mutex_create(&mutex, APR_THREAD_MUTEX_DEFAULT,
                                         m->pool);
        if (status != APR_SUCCESS) {
            apr_pool_destroy(m->pool);
            return NULL;
        }
        apr_allocator_mutex_set(allocator, mutex);

        status = apr_thread_mutex_create(&m->lock, APR_THREAD_MUTEX_DEFAULT,
                                         m->pool);
        if (status != APR_SUCCESS) {
            apr_pool_destroy(m->pool);
            return NULL;
        }
        
        m->max_streams = h2_config_sgeti(s, H2_CONF_MAX_STREAMS);
        m->stream_max_mem = h2_config_sgeti(s, H2_CONF_STREAM_MAX_MEM);

        m->streams = h2_ihash_create(m->pool, offsetof(h2_stream,id));
        m->sredo = h2_ihash_create(m->pool, offsetof(h2_stream,id));
        m->shold = h2_ihash_create(m->pool, offsetof(h2_stream,id));
        m->spurge = h2_ihash_create(m->pool, offsetof(h2_stream,id));
        m->q = h2_iq_create(m->pool, m->max_streams);

        status = h2_ififo_set_create(&m->readyq, m->pool, m->max_streams);
        if (status != APR_SUCCESS) {
            apr_pool_destroy(m->pool);
            return NULL;
        }

        m->workers = workers;
        m->max_active = workers->max_workers;
        m->limit_active = 6; /* the original h1 max parallel connections */
        m->last_limit_change = m->last_idle_block = apr_time_now();
        m->limit_change_interval = apr_time_from_msec(100);
        
        m->spare_slaves = apr_array_make(m->pool, 10, sizeof(conn_rec*));
    }
    return m;
}