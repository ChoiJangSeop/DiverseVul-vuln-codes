int hfi1_mmu_rb_register(void *ops_arg, struct mm_struct *mm,
			 struct mmu_rb_ops *ops,
			 struct workqueue_struct *wq,
			 struct mmu_rb_handler **handler)
{
	struct mmu_rb_handler *handlr;
	int ret;

	handlr = kmalloc(sizeof(*handlr), GFP_KERNEL);
	if (!handlr)
		return -ENOMEM;

	handlr->root = RB_ROOT_CACHED;
	handlr->ops = ops;
	handlr->ops_arg = ops_arg;
	INIT_HLIST_NODE(&handlr->mn.hlist);
	spin_lock_init(&handlr->lock);
	handlr->mn.ops = &mn_opts;
	handlr->mm = mm;
	INIT_WORK(&handlr->del_work, handle_remove);
	INIT_LIST_HEAD(&handlr->del_list);
	INIT_LIST_HEAD(&handlr->lru_list);
	handlr->wq = wq;

	ret = mmu_notifier_register(&handlr->mn, handlr->mm);
	if (ret) {
		kfree(handlr);
		return ret;
	}

	*handler = handlr;
	return 0;
}