apr_status_t h2_slave_run_pre_connection(conn_rec *slave, apr_socket_t *csd)
{
    if (slave->keepalives == 0) {
        /* Simulate that we had already a request on this connection. Some
         * hooks trigger special behaviour when keepalives is 0. 
         * (Not necessarily in pre_connection, but later. Set it here, so it
         * is in place.) */
        slave->keepalives = 1;
        /* We signal that this connection will be closed after the request.
         * Which is true in that sense that we throw away all traffic data
         * on this slave connection after each requests. Although we might
         * reuse internal structures like memory pools.
         * The wanted effect of this is that httpd does not try to clean up
         * any dangling data on this connection when a request is done. Which
         * is unneccessary on a h2 stream.
         */
        slave->keepalive = AP_CONN_CLOSE;
        return ap_run_pre_connection(slave, csd);
    }
    return APR_SUCCESS;
}