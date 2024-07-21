static void io_wqe_enqueue(struct io_wqe *wqe, struct io_wq_work *work)
{
	struct io_wqe_acct *acct = io_work_get_acct(wqe, work);
	unsigned work_flags = work->flags;
	bool do_create;

	/*
	 * If io-wq is exiting for this task, or if the request has explicitly
	 * been marked as one that should not get executed, cancel it here.
	 */
	if (test_bit(IO_WQ_BIT_EXIT, &wqe->wq->state) ||
	    (work->flags & IO_WQ_WORK_CANCEL)) {
run_cancel:
		io_run_cancel(work, wqe);
		return;
	}

	raw_spin_lock(&wqe->lock);
	io_wqe_insert_work(wqe, work);
	clear_bit(IO_ACCT_STALLED_BIT, &acct->flags);

	rcu_read_lock();
	do_create = !io_wqe_activate_free_worker(wqe, acct);
	rcu_read_unlock();

	raw_spin_unlock(&wqe->lock);

	if (do_create && ((work_flags & IO_WQ_WORK_CONCURRENT) ||
	    !atomic_read(&acct->nr_running))) {
		bool did_create;

		did_create = io_wqe_create_worker(wqe, acct);
		if (unlikely(!did_create)) {
			raw_spin_lock(&wqe->lock);
			/* fatal condition, failed to create the first worker */
			if (!acct->nr_workers) {
				raw_spin_unlock(&wqe->lock);
				goto run_cancel;
			}
			raw_spin_unlock(&wqe->lock);
		}
	}
}