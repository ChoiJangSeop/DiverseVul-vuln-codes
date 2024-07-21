innodb_bk_thread(
/*=============*/
	void*   arg)
{
	ENGINE_HANDLE*		handle;
	struct innodb_engine*	innodb_eng;
	innodb_conn_data_t*	conn_data;
	void*			thd = NULL;

	bk_thd_exited = false;

	handle = (ENGINE_HANDLE*) (arg);
	innodb_eng = innodb_handle(handle);

	if (innodb_eng->enable_binlog) {
		/* This thread will commit the transactions
		on behalf of the other threads. It will "pretend"
		to be each connection thread while doing it. */
		thd = handler_create_thd(true);
	}

	conn_data = UT_LIST_GET_FIRST(innodb_eng->conn_data);

	while(!memcached_shutdown) {
		innodb_conn_data_t*	next_conn_data;
		uint64_t                time;
		uint64_t		trx_start = 0;
		uint64_t		processed_count = 0;

		if (handler_check_global_read_lock_active()) {
			release_mdl_lock = true;
		} else {
			release_mdl_lock = false;
		}

		/* Do the cleanup every innodb_eng->bk_commit_interval
		seconds. We also check if the plugin is being shutdown
		every second */
		for (uint i = 0; i < innodb_eng->bk_commit_interval; i++) {
			sleep(1);

			/* If memcached is being shutdown, break */
			if (memcached_shutdown) {
				break;
			}
		}

		time = mci_get_time();

		if (UT_LIST_GET_LEN(innodb_eng->conn_data) == 0) {
			continue;
		}


		if (!conn_data) {
			conn_data = UT_LIST_GET_FIRST(innodb_eng->conn_data);
		}

		if (conn_data) {
			next_conn_data = UT_LIST_GET_NEXT(conn_list, conn_data);
		} else {
			next_conn_data = NULL;
		}

		/* Set the clean_stale_conn to prevent force clean in
		innodb_conn_clean. */
		LOCK_CONN_IF_NOT_LOCKED(false, innodb_eng);
		innodb_eng->clean_stale_conn = true;
		UNLOCK_CONN_IF_NOT_LOCKED(false, innodb_eng);

		while (conn_data) {
			if (release_mdl_lock && !conn_data->is_stale) {
				int err;

				if(conn_data->is_waiting_for_mdl) {
					goto next_item;
				}

				err = LOCK_CURRENT_CONN_TRYLOCK(conn_data);
				if (err != 0) {
					goto next_item;
				}
				/* We have got the lock here */
			} else {
				LOCK_CURRENT_CONN_IF_NOT_LOCKED(false, conn_data);
			}

			if (conn_data->is_stale) {
				UNLOCK_CURRENT_CONN_IF_NOT_LOCKED(
					false, conn_data);
				LOCK_CONN_IF_NOT_LOCKED(false, innodb_eng);
				UT_LIST_REMOVE(conn_list, innodb_eng->conn_data,
					       conn_data);
				UNLOCK_CONN_IF_NOT_LOCKED(false, innodb_eng);
				innodb_conn_clean_data(conn_data, false, true);
				goto next_item;
			}

			if (release_mdl_lock) {
				if (conn_data->thd) {
					handler_thd_attach(conn_data->thd, NULL);
				}

				if (conn_data->in_use) {
					UNLOCK_CURRENT_CONN_IF_NOT_LOCKED(false, conn_data);
					goto next_item;
				}

				innodb_reset_conn(conn_data, true, true,
					innodb_eng->enable_binlog);
				if(conn_data->mysql_tbl) {
					handler_unlock_table(conn_data->thd,
							     conn_data->mysql_tbl,
							     HDL_READ);
					conn_data->mysql_tbl = NULL;
				}

				/*Close the data cursor */
				if (conn_data->crsr) {
					innodb_cb_cursor_close(conn_data->crsr);
					conn_data->crsr = NULL;
				}
				if(conn_data->crsr_trx != NULL) {
					ib_cb_trx_release(conn_data->crsr_trx);
					conn_data->crsr_trx = NULL;
				}

				UNLOCK_CURRENT_CONN_IF_NOT_LOCKED(false, conn_data);
				goto next_item;
			}

			if (conn_data->crsr_trx) {
				trx_start = ib_cb_trx_get_start_time(
						conn_data->crsr_trx);
			}

			/* Check the trx, if it is qualified for
			reset and commit */
			if ((conn_data->n_writes_since_commit > 0
			     || conn_data->n_reads_since_commit > 0)
			    && trx_start
			    && (time - trx_start > CONN_IDLE_TIME_TO_BK_COMMIT)
			    && !conn_data->in_use) {
				/* binlog is running, make the thread
				attach to conn_data->thd for binlog
				committing */
				if (thd) {
					handler_thd_attach(
						conn_data->thd, NULL);
				}

				innodb_reset_conn(conn_data, true, true,
						  innodb_eng->enable_binlog);
				processed_count++;
			}

			UNLOCK_CURRENT_CONN_IF_NOT_LOCKED(false, conn_data);

next_item:
			conn_data = next_conn_data;

			/* Process BK_MAX_PROCESS_COMMIT (5) trx at a time */
			if (!release_mdl_lock &&
			    processed_count > BK_MAX_PROCESS_COMMIT) {
				break;
			}

			if (conn_data) {
				next_conn_data = UT_LIST_GET_NEXT(
					conn_list, conn_data);
			}
		}

		/* Set the clean_stale_conn back. */
		LOCK_CONN_IF_NOT_LOCKED(false, innodb_eng);
		innodb_eng->clean_stale_conn = false;
		UNLOCK_CONN_IF_NOT_LOCKED(false, innodb_eng);
	}

	bk_thd_exited = true;

	/* Change to its original state before close the MySQL THD */
	if (thd) {
		handler_thd_attach(thd, NULL);
		handler_close_thd(thd);
	}

	pthread_detach(pthread_self());
        pthread_exit(NULL);

	return((void*) 0);
}