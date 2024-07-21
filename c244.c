static void audit_inotify_unregister(struct list_head *in_list)
{
	struct audit_parent *p, *n;

	list_for_each_entry_safe(p, n, in_list, ilist) {
		list_del(&p->ilist);
		inotify_rm_watch(audit_ih, &p->wdata);
		/* the put matching the get in audit_do_del_rule() */
		put_inotify_watch(&p->wdata);
	}
}