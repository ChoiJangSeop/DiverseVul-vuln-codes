connection_threadmain(void *arg)
{
    Slapi_PBlock *pb = slapi_pblock_new();
    int32_t *snmp_vars_idx = (int32_t *) arg;
    /* wait forever for new pb until one is available or shutdown */
    int32_t interval = 0; /* used be  10 seconds */
    Connection *conn = NULL;
    Operation *op;
    ber_tag_t tag = 0;
    int thread_turbo_flag = 0;
    int ret = 0;
    int more_data = 0;
    int replication_connection = 0; /* If this connection is from a replication supplier, we want to ensure that operation processing is serialized */
    int doshutdown = 0;
    int maxthreads = 0;
    long bypasspollcnt = 0;

#if defined(hpux)
    /* Arrange to ignore SIGPIPE signals. */
    SIGNAL(SIGPIPE, SIG_IGN);
#endif
    thread_private_snmp_vars_set_idx(*snmp_vars_idx);

    while (1) {
        int is_timedout = 0;
        time_t curtime = 0;

        if (op_shutdown) {
            slapi_log_err(SLAPI_LOG_TRACE, "connection_threadmain",
                          "op_thread received shutdown signal\n");
            slapi_pblock_destroy(pb);
            g_decr_active_threadcnt();
            return;
        }

        if (!thread_turbo_flag && !more_data) {
	        Connection *pb_conn = NULL;

            /* If more data is left from the previous connection_read_operation,
               we should finish the op now.  Client might be thinking it's
               done sending the request and wait for the response forever.
               [blackflag 624234] */
            ret = connection_wait_for_new_work(pb, interval);

            switch (ret) {
            case CONN_NOWORK:
                PR_ASSERT(interval != 0); /* this should never happen */
                continue;
            case CONN_SHUTDOWN:
                slapi_log_err(SLAPI_LOG_TRACE, "connection_threadmain",
                              "op_thread received shutdown signal\n");
                slapi_pblock_destroy(pb);
                g_decr_active_threadcnt();
                return;
            case CONN_FOUND_WORK_TO_DO:
                /* note - don't need to lock here - connection should only
                   be used by this thread - since c_gettingber is set to 1
                   in connection_activity when the conn is added to the
                   work queue, setup_pr_read_pds won't add the connection prfd
                   to the poll list */
                slapi_pblock_get(pb, SLAPI_CONNECTION, &pb_conn);
                if (pb_conn == NULL) {
                    slapi_log_err(SLAPI_LOG_ERR, "connection_threadmain", "pb_conn is NULL\n");
                    slapi_pblock_destroy(pb);
                    g_decr_active_threadcnt();
                    return;
                }

                pthread_mutex_lock(&(pb_conn->c_mutex));
                if (pb_conn->c_anonlimits_set == 0) {
                    /*
                     * We have a new connection, set the anonymous reslimit idletimeout
                     * if applicable.
                     */
                    char *anon_dn = config_get_anon_limits_dn();
                    int idletimeout;
                    /* If an anonymous limits dn is set, use it to set the limits. */
                    if (anon_dn && (strlen(anon_dn) > 0)) {
                        Slapi_DN *anon_sdn = slapi_sdn_new_normdn_byref(anon_dn);
                        reslimit_update_from_dn(pb_conn, anon_sdn);
                        slapi_sdn_free(&anon_sdn);
                        if (slapi_reslimit_get_integer_limit(pb_conn,
                                                             pb_conn->c_idletimeout_handle,
                                                             &idletimeout) == SLAPI_RESLIMIT_STATUS_SUCCESS) {
                            pb_conn->c_idletimeout = idletimeout;
                        }
                    }
                    slapi_ch_free_string(&anon_dn);
                    /*
                     * Set connection as initialized to avoid setting anonymous limits
                     * multiple times on the same connection
                     */
                    pb_conn->c_anonlimits_set = 1;
                }

                /* must hold c_mutex so that it synchronizes the IO layer push
                 * with a potential pending sasl bind that is registering the IO layer
                 */
                if (connection_call_io_layer_callbacks(pb_conn)) {
                    slapi_log_err(SLAPI_LOG_ERR, "connection_threadmain",
                                  "Could not add/remove IO layers from connection\n");
                }
                pthread_mutex_unlock(&(pb_conn->c_mutex));
                break;
            default:
                break;
            }
        } else {

            /* The turbo mode may cause threads starvation.
               Do a yield here to reduce the starving
            */
            PR_Sleep(PR_INTERVAL_NO_WAIT);

            pthread_mutex_lock(&(conn->c_mutex));
            /* Make our own pb in turbo mode */
            connection_make_new_pb(pb, conn);
            if (connection_call_io_layer_callbacks(conn)) {
                slapi_log_err(SLAPI_LOG_ERR, "connection_threadmain",
                              "Could not add/remove IO layers from connection\n");
            }
            pthread_mutex_unlock(&(conn->c_mutex));
            if (!config_check_referral_mode()) {
                slapi_counter_increment(g_get_per_thread_snmp_vars()->server_tbl.dsOpInitiated);
                slapi_counter_increment(g_get_per_thread_snmp_vars()->ops_tbl.dsInOps);
            }
        }
        /* Once we're here we have a pb */
        slapi_pblock_get(pb, SLAPI_CONNECTION, &conn);
        slapi_pblock_get(pb, SLAPI_OPERATION, &op);
        if (conn == NULL || op == NULL) {
            slapi_log_err(SLAPI_LOG_ERR, "connection_threadmain", "NULL param: conn (0x%p) op (0x%p)\n", conn, op);
            slapi_pblock_destroy(pb);
            g_decr_active_threadcnt();
            return;
        }
        maxthreads = conn->c_max_threads_per_conn;
        more_data = 0;
        ret = connection_read_operation(conn, op, &tag, &more_data);
        if ((ret == CONN_DONE) || (ret == CONN_TIMEDOUT)) {
            slapi_log_err(SLAPI_LOG_CONNS, "connection_threadmain",
                          "conn %" PRIu64 " read not ready due to %d - thread_turbo_flag %d more_data %d "
                          "ops_initiated %d refcnt %d flags %d\n",
                          conn->c_connid, ret, thread_turbo_flag, more_data,
                          conn->c_opsinitiated, conn->c_refcnt, conn->c_flags);
        } else if (ret == CONN_FOUND_WORK_TO_DO) {
            slapi_log_err(SLAPI_LOG_CONNS, "connection_threadmain",
                          "conn %" PRIu64 " read operation successfully - thread_turbo_flag %d more_data %d "
                          "ops_initiated %d refcnt %d flags %d\n",
                          conn->c_connid, thread_turbo_flag, more_data,
                          conn->c_opsinitiated, conn->c_refcnt, conn->c_flags);
        }

        curtime = slapi_current_rel_time_t();
#define DB_PERF_TURBO 1
#if defined(DB_PERF_TURBO)
        /* If it's been a while since we last did it ... */
        if (curtime - conn->c_private->previous_count_check_time > CONN_TURBO_CHECK_INTERVAL) {
            if (config_get_enable_turbo_mode()) {
                int new_turbo_flag = 0;
                /* Check the connection's activity level */
                connection_check_activity_level(conn);
                /* And if appropriate, change into or out of turbo mode */
                connection_enter_leave_turbo(conn, thread_turbo_flag, &new_turbo_flag);
                thread_turbo_flag = new_turbo_flag;
            } else {
                thread_turbo_flag = 0;
            }
        }

        /* turn off turbo mode immediately if any pb waiting in global queue */
        if (thread_turbo_flag && !WORK_Q_EMPTY) {
            thread_turbo_flag = 0;
            slapi_log_err(SLAPI_LOG_CONNS, "connection_threadmain",
                          "conn %" PRIu64 " leaving turbo mode - pb_q is not empty %d\n",
                          conn->c_connid, work_q_size);
        }
#endif

        switch (ret) {
        case CONN_DONE:
        /* This means that the connection was closed, so clear turbo mode */
        /*FALLTHROUGH*/
        case CONN_TIMEDOUT:
            thread_turbo_flag = 0;
            is_timedout = 1;
            /* In the case of CONN_DONE, more_data could have been set to 1
                 * in connection_read_operation before an error was encountered.
                 * In that case, we need to set more_data to 0 - even if there is
                 * more data available, we're not going to use it anyway.
                 * In the case of CONN_TIMEDOUT, it is only used in one place, and
                 * more_data will never be set to 1, so it is safe to set it to 0 here.
                 * We need more_data to be 0 so the connection will be processed
                 * correctly at the end of this function.
                 */
            more_data = 0;
            /* note:
                 * should call connection_make_readable after the op is removed
                 * connection_make_readable(conn);
                 */
            slapi_log_err(SLAPI_LOG_CONNS, "connection_threadmain",
                          "conn %" PRIu64 " leaving turbo mode due to %d\n",
                          conn->c_connid, ret);
            goto done;
        case CONN_SHUTDOWN:
            slapi_log_err(SLAPI_LOG_TRACE, "connection_threadmain",
                          "op_thread received shutdown signal\n");
            g_decr_active_threadcnt();
            doshutdown = 1;
            goto done; /* To destroy pb, jump to done once */
        default:
            break;
        }

        /* if we got here, then we had some read activity */
        if (thread_turbo_flag) {
            /* turbo mode avoids handle_pr_read_ready which avoids setting c_idlesince
               update c_idlesince here since, if we got some read activity, we are
               not idle */
            conn->c_idlesince = curtime;
        }

        /*
         * Do not put the connection back to the read ready poll list
         * if the operation is unbind.  Unbind will close the socket.
         * Similarly, if we are in turbo mode, don't send the socket
         * back to the poll set.
         * more_data: [blackflag 624234]
         * If the connection is from a replication supplier, don't make it readable here.
         * We want to ensure that replication operations are processed strictly in the order
         * they are received off the wire.
         */
        replication_connection = conn->c_isreplication_session;
        if ((tag != LDAP_REQ_UNBIND) && !thread_turbo_flag && !replication_connection) {
            if (!more_data) {
                conn->c_flags &= ~CONN_FLAG_MAX_THREADS;
                pthread_mutex_lock(&(conn->c_mutex));
                connection_make_readable_nolock(conn);
                pthread_mutex_unlock(&(conn->c_mutex));
                /* once the connection is readable, another thread may access conn,
                 * so need locking from here on */
                signal_listner();
            } else { /* more data in conn - just put back on work_q - bypass poll */
                bypasspollcnt++;
                pthread_mutex_lock(&(conn->c_mutex));
                /* don't do this if it would put us over the max threads per conn */
                if (conn->c_threadnumber < maxthreads) {
                    /* for turbo, c_idlesince is set above - for !turbo and
                     * !more_data, we put the conn back in the poll loop and
                     * c_idlesince is set in handle_pr_read_ready - since we
                     * are bypassing both of those, we set idlesince here
                     */
                    conn->c_idlesince = curtime;
                    connection_activity(conn, maxthreads);
                    slapi_log_err(SLAPI_LOG_CONNS, "connection_threadmain", "conn %" PRIu64 " queued because more_data\n",
                                  conn->c_connid);
                } else {
                    /* keep count of how many times maxthreads has blocked an operation */
                    conn->c_maxthreadsblocked++;
                    if (conn->c_maxthreadsblocked == 1 && connection_has_psearch(conn)) {
                        slapi_log_err(SLAPI_LOG_NOTICE, "connection_threadmain",
                                "Connection (conn=%" PRIu64 ") has a running persistent search "
                                "that has exceeded the maximum allowed threads per connection. "
                                "New operations will be blocked.\n",
                                conn->c_connid);
                    }
                }
                pthread_mutex_unlock(&(conn->c_mutex));
            }
        }

        /* are we in referral-only mode? */
        if (config_check_referral_mode() && tag != LDAP_REQ_UNBIND) {
            referral_mode_reply(pb);
            goto done;
        }

        /* check if new password is required */
        if (connection_need_new_password(conn, op, pb)) {
            goto done;
        }

        /* if this is a bulk import, only "add" and "import done"
         * are allowed */
        if (conn->c_flags & CONN_FLAG_IMPORT) {
            if ((tag != LDAP_REQ_ADD) && (tag != LDAP_REQ_EXTENDED)) {
                /* no cookie for you. */
                slapi_log_err(SLAPI_LOG_ERR, "connection_threadmain", "Attempted operation %lu "
                                                                      "from within bulk import\n",
                              tag);
                slapi_send_ldap_result(pb, LDAP_PROTOCOL_ERROR, NULL,
                                       NULL, 0, NULL);
                goto done;
            }
        }

        /*
         * Fix bz 1931820 issue (the check to set OP_FLAG_REPLICATED may be done
         * before replication session is properly set).
         */
        if (replication_connection) {
            operation_set_flag(op, OP_FLAG_REPLICATED);
        }

        /*
         * Call the do_<operation> function to process this request.
         */
        connection_dispatch_operation(conn, op, pb);

    done:
        if (doshutdown) {
            pthread_mutex_lock(&(conn->c_mutex));
            connection_remove_operation_ext(pb, conn, op);
            connection_make_readable_nolock(conn);
            conn->c_threadnumber--;
            slapi_counter_decrement(conns_in_maxthreads);
            slapi_counter_decrement(g_get_per_thread_snmp_vars()->ops_tbl.dsConnectionsInMaxThreads);
            connection_release_nolock(conn);
            pthread_mutex_unlock(&(conn->c_mutex));
            signal_listner();
            slapi_pblock_destroy(pb);
            return;
        }
        /*
         * done with this operation. delete it from the op
         * queue for this connection, delete the number of
         * threads devoted to this connection, and see if
         * there's more work to do right now on this conn.
         */

        /* number of ops on this connection */
        PR_AtomicIncrement(&conn->c_opscompleted);
        /* total number of ops for the server */
        slapi_counter_increment(g_get_per_thread_snmp_vars()->server_tbl.dsOpCompleted);
        /* If this op isn't a persistent search, remove it */
        if (op->o_flags & OP_FLAG_PS) {
            /* Release the connection (i.e. decrease refcnt) at the condition
             * this thread will not loop on it.
             * If we are in turbo mode (dedicated to that connection) or
             * more_data (continue reading buffered req) this thread
             * continues to hold the connection
             */
            if (!thread_turbo_flag && !more_data) {
                pthread_mutex_lock(&(conn->c_mutex));
                connection_release_nolock(conn); /* psearch acquires ref to conn - release this one now */
                pthread_mutex_unlock(&(conn->c_mutex));
            }
            /* ps_add makes a shallow copy of the pb - so we
                 * can't free it or init it here - just set operation to NULL.
                 * ps_send_results will call connection_remove_operation_ext to free it
                 */
            slapi_pblock_set(pb, SLAPI_OPERATION, NULL);
            slapi_pblock_init(pb);
        } else {
            /* delete from connection operation queue & decr refcnt */
            int conn_closed = 0;
            pthread_mutex_lock(&(conn->c_mutex));
            connection_remove_operation_ext(pb, conn, op);

            /* If we're in turbo mode, we keep our reference to the connection alive */
            /* can't use the more_data var because connection could have changed in another thread */
            slapi_log_err(SLAPI_LOG_CONNS, "connection_threadmain", "conn %" PRIu64 " check more_data %d thread_turbo_flag %d"
                          "repl_conn_bef %d, repl_conn_now %d\n",
                          conn->c_connid, more_data, thread_turbo_flag,
                          replication_connection, conn->c_isreplication_session);
            if (!replication_connection &&  conn->c_isreplication_session) {
                /* it a connection that was just flagged as replication connection */
                more_data = 0;
            } else {
                /* normal connection or already established replication connection */
                more_data = conn_buffered_data_avail_nolock(conn, &conn_closed) ? 1 : 0;
            }
            if (!more_data) {
                if (!thread_turbo_flag) {
                    int32_t need_wakeup = 0;

                    /*
                     * Don't release the connection now.
                     * But note down what to do.
                     */
                    if (replication_connection || (1 == is_timedout)) {
                        connection_make_readable_nolock(conn);
                        need_wakeup = 1;
                    }
                    if (!need_wakeup) {
                        if (conn->c_threadnumber == maxthreads) {
                            need_wakeup = 1;
                        } else {
                            need_wakeup = 0;
                        }
                    }

                    if (conn->c_threadnumber == maxthreads) {
                        conn->c_flags &= ~CONN_FLAG_MAX_THREADS;
                        slapi_counter_decrement(conns_in_maxthreads);
                        slapi_counter_decrement(g_get_per_thread_snmp_vars()->ops_tbl.dsConnectionsInMaxThreads);
                    }
                    conn->c_threadnumber--;
                    connection_release_nolock(conn);
                    /* If need_wakeup, call signal_listner once.
                     * Need to release the connection (refcnt--)
                     * before that call.
                     */
                    if (need_wakeup) {
                        signal_listner();
                        need_wakeup = 0;
                    }
                } else if (1 == is_timedout) {
                    /* covscan reports this code is unreachable  (2019/6/4) */
                    connection_make_readable_nolock(conn);
                    signal_listner();
                }
            }
            pthread_mutex_unlock(&(conn->c_mutex));
        }
    } /* while (1) */
}