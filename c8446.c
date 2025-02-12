struct io_wq *io_wq_create(unsigned bounded, struct io_wq_data *data)
{
	int ret = -ENOMEM, i, node;
	struct io_wq *wq;

	wq = kcalloc(1, sizeof(*wq), GFP_KERNEL);
	if (!wq)
		return ERR_PTR(-ENOMEM);

	wq->nr_wqes = num_online_nodes();
	wq->wqes = kcalloc(wq->nr_wqes, sizeof(struct io_wqe *), GFP_KERNEL);
	if (!wq->wqes) {
		kfree(wq);
		return ERR_PTR(-ENOMEM);
	}

	wq->get_work = data->get_work;
	wq->put_work = data->put_work;

	/* caller must already hold a reference to this */
	wq->user = data->user;

	i = 0;
	for_each_online_node(node) {
		struct io_wqe *wqe;

		wqe = kcalloc_node(1, sizeof(struct io_wqe), GFP_KERNEL, node);
		if (!wqe)
			break;
		wq->wqes[i] = wqe;
		wqe->node = node;
		wqe->acct[IO_WQ_ACCT_BOUND].max_workers = bounded;
		atomic_set(&wqe->acct[IO_WQ_ACCT_BOUND].nr_running, 0);
		if (wq->user) {
			wqe->acct[IO_WQ_ACCT_UNBOUND].max_workers =
					task_rlimit(current, RLIMIT_NPROC);
		}
		atomic_set(&wqe->acct[IO_WQ_ACCT_UNBOUND].nr_running, 0);
		wqe->node = node;
		wqe->wq = wq;
		spin_lock_init(&wqe->lock);
		INIT_LIST_HEAD(&wqe->work_list);
		INIT_HLIST_NULLS_HEAD(&wqe->free_list, 0);
		INIT_HLIST_NULLS_HEAD(&wqe->busy_list, 1);
		INIT_LIST_HEAD(&wqe->all_list);

		i++;
	}

	init_completion(&wq->done);

	if (i != wq->nr_wqes)
		goto err;

	/* caller must have already done mmgrab() on this mm */
	wq->mm = data->mm;

	wq->manager = kthread_create(io_wq_manager, wq, "io_wq_manager");
	if (!IS_ERR(wq->manager)) {
		wake_up_process(wq->manager);
		wait_for_completion(&wq->done);
		if (test_bit(IO_WQ_BIT_ERROR, &wq->state)) {
			ret = -ENOMEM;
			goto err;
		}
		reinit_completion(&wq->done);
		return wq;
	}

	ret = PTR_ERR(wq->manager);
	complete(&wq->done);
err:
	for (i = 0; i < wq->nr_wqes; i++)
		kfree(wq->wqes[i]);
	kfree(wq->wqes);
	kfree(wq);
	return ERR_PTR(ret);
}