	__releases(wqe->lock)
{
	struct io_wq_work *work, *old_work = NULL, *put_work = NULL;
	struct io_wqe *wqe = worker->wqe;
	struct io_wq *wq = wqe->wq;

	do {
		unsigned hash = -1U;

		/*
		 * If we got some work, mark us as busy. If we didn't, but
		 * the list isn't empty, it means we stalled on hashed work.
		 * Mark us stalled so we don't keep looking for work when we
		 * can't make progress, any work completion or insertion will
		 * clear the stalled flag.
		 */
		work = io_get_next_work(wqe, &hash);
		if (work)
			__io_worker_busy(wqe, worker, work);
		else if (!list_empty(&wqe->work_list))
			wqe->flags |= IO_WQE_FLAG_STALLED;

		spin_unlock_irq(&wqe->lock);
		if (put_work && wq->put_work)
			wq->put_work(old_work);
		if (!work)
			break;
next:
		/* flush any pending signals before assigning new work */
		if (signal_pending(current))
			flush_signals(current);

		spin_lock_irq(&worker->lock);
		worker->cur_work = work;
		spin_unlock_irq(&worker->lock);

		if (work->flags & IO_WQ_WORK_CB)
			work->func(&work);

		if ((work->flags & IO_WQ_WORK_NEEDS_FILES) &&
		    current->files != work->files) {
			task_lock(current);
			current->files = work->files;
			task_unlock(current);
		}
		if ((work->flags & IO_WQ_WORK_NEEDS_USER) && !worker->mm &&
		    wq->mm && mmget_not_zero(wq->mm)) {
			use_mm(wq->mm);
			set_fs(USER_DS);
			worker->mm = wq->mm;
		}
		if (test_bit(IO_WQ_BIT_CANCEL, &wq->state))
			work->flags |= IO_WQ_WORK_CANCEL;
		if (worker->mm)
			work->flags |= IO_WQ_WORK_HAS_MM;

		if (wq->get_work && !(work->flags & IO_WQ_WORK_INTERNAL)) {
			put_work = work;
			wq->get_work(work);
		}

		old_work = work;
		work->func(&work);

		spin_lock_irq(&worker->lock);
		worker->cur_work = NULL;
		spin_unlock_irq(&worker->lock);

		spin_lock_irq(&wqe->lock);

		if (hash != -1U) {
			wqe->hash_map &= ~BIT_ULL(hash);
			wqe->flags &= ~IO_WQE_FLAG_STALLED;
		}
		if (work && work != old_work) {
			spin_unlock_irq(&wqe->lock);

			if (put_work && wq->put_work) {
				wq->put_work(put_work);
				put_work = NULL;
			}

			/* dependent work not hashed */
			hash = -1U;
			goto next;
		}
	} while (1);
}