static int xenbus_write_transaction(unsigned msg_type,
				    struct xenbus_file_priv *u)
{
	int rc;
	void *reply;
	struct xenbus_transaction_holder *trans = NULL;
	LIST_HEAD(staging_q);

	if (msg_type == XS_TRANSACTION_START) {
		trans = kmalloc(sizeof(*trans), GFP_KERNEL);
		if (!trans) {
			rc = -ENOMEM;
			goto out;
		}
	}

	reply = xenbus_dev_request_and_reply(&u->u.msg);
	if (IS_ERR(reply)) {
		kfree(trans);
		rc = PTR_ERR(reply);
		goto out;
	}

	if (msg_type == XS_TRANSACTION_START) {
		if (u->u.msg.type == XS_ERROR)
			kfree(trans);
		else {
			trans->handle.id = simple_strtoul(reply, NULL, 0);
			list_add(&trans->list, &u->transactions);
		}
	} else if (u->u.msg.type == XS_TRANSACTION_END) {
		list_for_each_entry(trans, &u->transactions, list)
			if (trans->handle.id == u->u.msg.tx_id)
				break;
		BUG_ON(&trans->list == &u->transactions);
		list_del(&trans->list);

		kfree(trans);
	}

	mutex_lock(&u->reply_mutex);
	rc = queue_reply(&staging_q, &u->u.msg, sizeof(u->u.msg));
	if (!rc)
		rc = queue_reply(&staging_q, reply, u->u.msg.len);
	if (!rc) {
		list_splice_tail(&staging_q, &u->read_buffers);
		wake_up(&u->read_waitq);
	} else {
		queue_cleanup(&staging_q);
	}
	mutex_unlock(&u->reply_mutex);

	kfree(reply);

out:
	return rc;
}