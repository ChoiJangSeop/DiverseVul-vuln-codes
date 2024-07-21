static void binder_transaction(struct binder_proc *proc,
			       struct binder_thread *thread,
			       struct binder_transaction_data *tr, int reply,
			       binder_size_t extra_buffers_size)
{
	int ret;
	struct binder_transaction *t;
	struct binder_work *w;
	struct binder_work *tcomplete;
	binder_size_t *offp, *off_end, *off_start;
	binder_size_t off_min;
	u8 *sg_bufp, *sg_buf_end;
	struct binder_proc *target_proc = NULL;
	struct binder_thread *target_thread = NULL;
	struct binder_node *target_node = NULL;
	struct binder_transaction *in_reply_to = NULL;
	struct binder_transaction_log_entry *e;
	uint32_t return_error = 0;
	uint32_t return_error_param = 0;
	uint32_t return_error_line = 0;
	struct binder_buffer_object *last_fixup_obj = NULL;
	binder_size_t last_fixup_min_off = 0;
	struct binder_context *context = proc->context;
	int t_debug_id = atomic_inc_return(&binder_last_id);

	e = binder_transaction_log_add(&binder_transaction_log);
	e->debug_id = t_debug_id;
	e->call_type = reply ? 2 : !!(tr->flags & TF_ONE_WAY);
	e->from_proc = proc->pid;
	e->from_thread = thread->pid;
	e->target_handle = tr->target.handle;
	e->data_size = tr->data_size;
	e->offsets_size = tr->offsets_size;
	e->context_name = proc->context->name;

	if (reply) {
		binder_inner_proc_lock(proc);
		in_reply_to = thread->transaction_stack;
		if (in_reply_to == NULL) {
			binder_inner_proc_unlock(proc);
			binder_user_error("%d:%d got reply transaction with no transaction stack\n",
					  proc->pid, thread->pid);
			return_error = BR_FAILED_REPLY;
			return_error_param = -EPROTO;
			return_error_line = __LINE__;
			goto err_empty_call_stack;
		}
		if (in_reply_to->to_thread != thread) {
			spin_lock(&in_reply_to->lock);
			binder_user_error("%d:%d got reply transaction with bad transaction stack, transaction %d has target %d:%d\n",
				proc->pid, thread->pid, in_reply_to->debug_id,
				in_reply_to->to_proc ?
				in_reply_to->to_proc->pid : 0,
				in_reply_to->to_thread ?
				in_reply_to->to_thread->pid : 0);
			spin_unlock(&in_reply_to->lock);
			binder_inner_proc_unlock(proc);
			return_error = BR_FAILED_REPLY;
			return_error_param = -EPROTO;
			return_error_line = __LINE__;
			in_reply_to = NULL;
			goto err_bad_call_stack;
		}
		thread->transaction_stack = in_reply_to->to_parent;
		binder_inner_proc_unlock(proc);
		binder_set_nice(in_reply_to->saved_priority);
		target_thread = binder_get_txn_from_and_acq_inner(in_reply_to);
		if (target_thread == NULL) {
			return_error = BR_DEAD_REPLY;
			return_error_line = __LINE__;
			goto err_dead_binder;
		}
		if (target_thread->transaction_stack != in_reply_to) {
			binder_user_error("%d:%d got reply transaction with bad target transaction stack %d, expected %d\n",
				proc->pid, thread->pid,
				target_thread->transaction_stack ?
				target_thread->transaction_stack->debug_id : 0,
				in_reply_to->debug_id);
			binder_inner_proc_unlock(target_thread->proc);
			return_error = BR_FAILED_REPLY;
			return_error_param = -EPROTO;
			return_error_line = __LINE__;
			in_reply_to = NULL;
			target_thread = NULL;
			goto err_dead_binder;
		}
		target_proc = target_thread->proc;
		target_proc->tmp_ref++;
		binder_inner_proc_unlock(target_thread->proc);
	} else {
		if (tr->target.handle) {
			struct binder_ref *ref;

			/*
			 * There must already be a strong ref
			 * on this node. If so, do a strong
			 * increment on the node to ensure it
			 * stays alive until the transaction is
			 * done.
			 */
			binder_proc_lock(proc);
			ref = binder_get_ref_olocked(proc, tr->target.handle,
						     true);
			if (ref) {
				target_node = binder_get_node_refs_for_txn(
						ref->node, &target_proc,
						&return_error);
			} else {
				binder_user_error("%d:%d got transaction to invalid handle\n",
						  proc->pid, thread->pid);
				return_error = BR_FAILED_REPLY;
			}
			binder_proc_unlock(proc);
		} else {
			mutex_lock(&context->context_mgr_node_lock);
			target_node = context->binder_context_mgr_node;
			if (target_node)
				target_node = binder_get_node_refs_for_txn(
						target_node, &target_proc,
						&return_error);
			else
				return_error = BR_DEAD_REPLY;
			mutex_unlock(&context->context_mgr_node_lock);
			if (target_node && target_proc == proc) {
				binder_user_error("%d:%d got transaction to context manager from process owning it\n",
						  proc->pid, thread->pid);
				return_error = BR_FAILED_REPLY;
				return_error_param = -EINVAL;
				return_error_line = __LINE__;
				goto err_invalid_target_handle;
			}
		}
		if (!target_node) {
			/*
			 * return_error is set above
			 */
			return_error_param = -EINVAL;
			return_error_line = __LINE__;
			goto err_dead_binder;
		}
		e->to_node = target_node->debug_id;
		if (security_binder_transaction(proc->tsk,
						target_proc->tsk) < 0) {
			return_error = BR_FAILED_REPLY;
			return_error_param = -EPERM;
			return_error_line = __LINE__;
			goto err_invalid_target_handle;
		}
		binder_inner_proc_lock(proc);

		w = list_first_entry_or_null(&thread->todo,
					     struct binder_work, entry);
		if (!(tr->flags & TF_ONE_WAY) && w &&
		    w->type == BINDER_WORK_TRANSACTION) {
			/*
			 * Do not allow new outgoing transaction from a
			 * thread that has a transaction at the head of
			 * its todo list. Only need to check the head
			 * because binder_select_thread_ilocked picks a
			 * thread from proc->waiting_threads to enqueue
			 * the transaction, and nothing is queued to the
			 * todo list while the thread is on waiting_threads.
			 */
			binder_user_error("%d:%d new transaction not allowed when there is a transaction on thread todo\n",
					  proc->pid, thread->pid);
			binder_inner_proc_unlock(proc);
			return_error = BR_FAILED_REPLY;
			return_error_param = -EPROTO;
			return_error_line = __LINE__;
			goto err_bad_todo_list;
		}

		if (!(tr->flags & TF_ONE_WAY) && thread->transaction_stack) {
			struct binder_transaction *tmp;

			tmp = thread->transaction_stack;
			if (tmp->to_thread != thread) {
				spin_lock(&tmp->lock);
				binder_user_error("%d:%d got new transaction with bad transaction stack, transaction %d has target %d:%d\n",
					proc->pid, thread->pid, tmp->debug_id,
					tmp->to_proc ? tmp->to_proc->pid : 0,
					tmp->to_thread ?
					tmp->to_thread->pid : 0);
				spin_unlock(&tmp->lock);
				binder_inner_proc_unlock(proc);
				return_error = BR_FAILED_REPLY;
				return_error_param = -EPROTO;
				return_error_line = __LINE__;
				goto err_bad_call_stack;
			}
			while (tmp) {
				struct binder_thread *from;

				spin_lock(&tmp->lock);
				from = tmp->from;
				if (from && from->proc == target_proc) {
					atomic_inc(&from->tmp_ref);
					target_thread = from;
					spin_unlock(&tmp->lock);
					break;
				}
				spin_unlock(&tmp->lock);
				tmp = tmp->from_parent;
			}
		}
		binder_inner_proc_unlock(proc);
	}
	if (target_thread)
		e->to_thread = target_thread->pid;
	e->to_proc = target_proc->pid;

	/* TODO: reuse incoming transaction for reply */
	t = kzalloc(sizeof(*t), GFP_KERNEL);
	if (t == NULL) {
		return_error = BR_FAILED_REPLY;
		return_error_param = -ENOMEM;
		return_error_line = __LINE__;
		goto err_alloc_t_failed;
	}
	INIT_LIST_HEAD(&t->fd_fixups);
	binder_stats_created(BINDER_STAT_TRANSACTION);
	spin_lock_init(&t->lock);

	tcomplete = kzalloc(sizeof(*tcomplete), GFP_KERNEL);
	if (tcomplete == NULL) {
		return_error = BR_FAILED_REPLY;
		return_error_param = -ENOMEM;
		return_error_line = __LINE__;
		goto err_alloc_tcomplete_failed;
	}
	binder_stats_created(BINDER_STAT_TRANSACTION_COMPLETE);

	t->debug_id = t_debug_id;

	if (reply)
		binder_debug(BINDER_DEBUG_TRANSACTION,
			     "%d:%d BC_REPLY %d -> %d:%d, data %016llx-%016llx size %lld-%lld-%lld\n",
			     proc->pid, thread->pid, t->debug_id,
			     target_proc->pid, target_thread->pid,
			     (u64)tr->data.ptr.buffer,
			     (u64)tr->data.ptr.offsets,
			     (u64)tr->data_size, (u64)tr->offsets_size,
			     (u64)extra_buffers_size);
	else
		binder_debug(BINDER_DEBUG_TRANSACTION,
			     "%d:%d BC_TRANSACTION %d -> %d - node %d, data %016llx-%016llx size %lld-%lld-%lld\n",
			     proc->pid, thread->pid, t->debug_id,
			     target_proc->pid, target_node->debug_id,
			     (u64)tr->data.ptr.buffer,
			     (u64)tr->data.ptr.offsets,
			     (u64)tr->data_size, (u64)tr->offsets_size,
			     (u64)extra_buffers_size);

	if (!reply && !(tr->flags & TF_ONE_WAY))
		t->from = thread;
	else
		t->from = NULL;
	t->sender_euid = task_euid(proc->tsk);
	t->to_proc = target_proc;
	t->to_thread = target_thread;
	t->code = tr->code;
	t->flags = tr->flags;
	t->priority = task_nice(current);

	trace_binder_transaction(reply, t, target_node);

	t->buffer = binder_alloc_new_buf(&target_proc->alloc, tr->data_size,
		tr->offsets_size, extra_buffers_size,
		!reply && (t->flags & TF_ONE_WAY));
	if (IS_ERR(t->buffer)) {
		/*
		 * -ESRCH indicates VMA cleared. The target is dying.
		 */
		return_error_param = PTR_ERR(t->buffer);
		return_error = return_error_param == -ESRCH ?
			BR_DEAD_REPLY : BR_FAILED_REPLY;
		return_error_line = __LINE__;
		t->buffer = NULL;
		goto err_binder_alloc_buf_failed;
	}
	t->buffer->allow_user_free = 0;
	t->buffer->debug_id = t->debug_id;
	t->buffer->transaction = t;
	t->buffer->target_node = target_node;
	trace_binder_transaction_alloc_buf(t->buffer);
	off_start = (binder_size_t *)(t->buffer->data +
				      ALIGN(tr->data_size, sizeof(void *)));
	offp = off_start;

	if (copy_from_user(t->buffer->data, (const void __user *)(uintptr_t)
			   tr->data.ptr.buffer, tr->data_size)) {
		binder_user_error("%d:%d got transaction with invalid data ptr\n",
				proc->pid, thread->pid);
		return_error = BR_FAILED_REPLY;
		return_error_param = -EFAULT;
		return_error_line = __LINE__;
		goto err_copy_data_failed;
	}
	if (copy_from_user(offp, (const void __user *)(uintptr_t)
			   tr->data.ptr.offsets, tr->offsets_size)) {
		binder_user_error("%d:%d got transaction with invalid offsets ptr\n",
				proc->pid, thread->pid);
		return_error = BR_FAILED_REPLY;
		return_error_param = -EFAULT;
		return_error_line = __LINE__;
		goto err_copy_data_failed;
	}
	if (!IS_ALIGNED(tr->offsets_size, sizeof(binder_size_t))) {
		binder_user_error("%d:%d got transaction with invalid offsets size, %lld\n",
				proc->pid, thread->pid, (u64)tr->offsets_size);
		return_error = BR_FAILED_REPLY;
		return_error_param = -EINVAL;
		return_error_line = __LINE__;
		goto err_bad_offset;
	}
	if (!IS_ALIGNED(extra_buffers_size, sizeof(u64))) {
		binder_user_error("%d:%d got transaction with unaligned buffers size, %lld\n",
				  proc->pid, thread->pid,
				  (u64)extra_buffers_size);
		return_error = BR_FAILED_REPLY;
		return_error_param = -EINVAL;
		return_error_line = __LINE__;
		goto err_bad_offset;
	}
	off_end = (void *)off_start + tr->offsets_size;
	sg_bufp = (u8 *)(PTR_ALIGN(off_end, sizeof(void *)));
	sg_buf_end = sg_bufp + extra_buffers_size;
	off_min = 0;
	for (; offp < off_end; offp++) {
		struct binder_object_header *hdr;
		size_t object_size = binder_validate_object(t->buffer, *offp);

		if (object_size == 0 || *offp < off_min) {
			binder_user_error("%d:%d got transaction with invalid offset (%lld, min %lld max %lld) or object.\n",
					  proc->pid, thread->pid, (u64)*offp,
					  (u64)off_min,
					  (u64)t->buffer->data_size);
			return_error = BR_FAILED_REPLY;
			return_error_param = -EINVAL;
			return_error_line = __LINE__;
			goto err_bad_offset;
		}

		hdr = (struct binder_object_header *)(t->buffer->data + *offp);
		off_min = *offp + object_size;
		switch (hdr->type) {
		case BINDER_TYPE_BINDER:
		case BINDER_TYPE_WEAK_BINDER: {
			struct flat_binder_object *fp;

			fp = to_flat_binder_object(hdr);
			ret = binder_translate_binder(fp, t, thread);
			if (ret < 0) {
				return_error = BR_FAILED_REPLY;
				return_error_param = ret;
				return_error_line = __LINE__;
				goto err_translate_failed;
			}
		} break;
		case BINDER_TYPE_HANDLE:
		case BINDER_TYPE_WEAK_HANDLE: {
			struct flat_binder_object *fp;

			fp = to_flat_binder_object(hdr);
			ret = binder_translate_handle(fp, t, thread);
			if (ret < 0) {
				return_error = BR_FAILED_REPLY;
				return_error_param = ret;
				return_error_line = __LINE__;
				goto err_translate_failed;
			}
		} break;

		case BINDER_TYPE_FD: {
			struct binder_fd_object *fp = to_binder_fd_object(hdr);
			int ret = binder_translate_fd(&fp->fd, t, thread,
						      in_reply_to);

			if (ret < 0) {
				return_error = BR_FAILED_REPLY;
				return_error_param = ret;
				return_error_line = __LINE__;
				goto err_translate_failed;
			}
			fp->pad_binder = 0;
		} break;
		case BINDER_TYPE_FDA: {
			struct binder_fd_array_object *fda =
				to_binder_fd_array_object(hdr);
			struct binder_buffer_object *parent =
				binder_validate_ptr(t->buffer, fda->parent,
						    off_start,
						    offp - off_start);
			if (!parent) {
				binder_user_error("%d:%d got transaction with invalid parent offset or type\n",
						  proc->pid, thread->pid);
				return_error = BR_FAILED_REPLY;
				return_error_param = -EINVAL;
				return_error_line = __LINE__;
				goto err_bad_parent;
			}
			if (!binder_validate_fixup(t->buffer, off_start,
						   parent, fda->parent_offset,
						   last_fixup_obj,
						   last_fixup_min_off)) {
				binder_user_error("%d:%d got transaction with out-of-order buffer fixup\n",
						  proc->pid, thread->pid);
				return_error = BR_FAILED_REPLY;
				return_error_param = -EINVAL;
				return_error_line = __LINE__;
				goto err_bad_parent;
			}
			ret = binder_translate_fd_array(fda, parent, t, thread,
							in_reply_to);
			if (ret < 0) {
				return_error = BR_FAILED_REPLY;
				return_error_param = ret;
				return_error_line = __LINE__;
				goto err_translate_failed;
			}
			last_fixup_obj = parent;
			last_fixup_min_off =
				fda->parent_offset + sizeof(u32) * fda->num_fds;
		} break;
		case BINDER_TYPE_PTR: {
			struct binder_buffer_object *bp =
				to_binder_buffer_object(hdr);
			size_t buf_left = sg_buf_end - sg_bufp;

			if (bp->length > buf_left) {
				binder_user_error("%d:%d got transaction with too large buffer\n",
						  proc->pid, thread->pid);
				return_error = BR_FAILED_REPLY;
				return_error_param = -EINVAL;
				return_error_line = __LINE__;
				goto err_bad_offset;
			}
			if (copy_from_user(sg_bufp,
					   (const void __user *)(uintptr_t)
					   bp->buffer, bp->length)) {
				binder_user_error("%d:%d got transaction with invalid offsets ptr\n",
						  proc->pid, thread->pid);
				return_error_param = -EFAULT;
				return_error = BR_FAILED_REPLY;
				return_error_line = __LINE__;
				goto err_copy_data_failed;
			}
			/* Fixup buffer pointer to target proc address space */
			bp->buffer = (uintptr_t)sg_bufp +
				binder_alloc_get_user_buffer_offset(
						&target_proc->alloc);
			sg_bufp += ALIGN(bp->length, sizeof(u64));

			ret = binder_fixup_parent(t, thread, bp, off_start,
						  offp - off_start,
						  last_fixup_obj,
						  last_fixup_min_off);
			if (ret < 0) {
				return_error = BR_FAILED_REPLY;
				return_error_param = ret;
				return_error_line = __LINE__;
				goto err_translate_failed;
			}
			last_fixup_obj = bp;
			last_fixup_min_off = 0;
		} break;
		default:
			binder_user_error("%d:%d got transaction with invalid object type, %x\n",
				proc->pid, thread->pid, hdr->type);
			return_error = BR_FAILED_REPLY;
			return_error_param = -EINVAL;
			return_error_line = __LINE__;
			goto err_bad_object_type;
		}
	}
	tcomplete->type = BINDER_WORK_TRANSACTION_COMPLETE;
	t->work.type = BINDER_WORK_TRANSACTION;

	if (reply) {
		binder_enqueue_thread_work(thread, tcomplete);
		binder_inner_proc_lock(target_proc);
		if (target_thread->is_dead) {
			binder_inner_proc_unlock(target_proc);
			goto err_dead_proc_or_thread;
		}
		BUG_ON(t->buffer->async_transaction != 0);
		binder_pop_transaction_ilocked(target_thread, in_reply_to);
		binder_enqueue_thread_work_ilocked(target_thread, &t->work);
		binder_inner_proc_unlock(target_proc);
		wake_up_interruptible_sync(&target_thread->wait);
		binder_free_transaction(in_reply_to);
	} else if (!(t->flags & TF_ONE_WAY)) {
		BUG_ON(t->buffer->async_transaction != 0);
		binder_inner_proc_lock(proc);
		/*
		 * Defer the TRANSACTION_COMPLETE, so we don't return to
		 * userspace immediately; this allows the target process to
		 * immediately start processing this transaction, reducing
		 * latency. We will then return the TRANSACTION_COMPLETE when
		 * the target replies (or there is an error).
		 */
		binder_enqueue_deferred_thread_work_ilocked(thread, tcomplete);
		t->need_reply = 1;
		t->from_parent = thread->transaction_stack;
		thread->transaction_stack = t;
		binder_inner_proc_unlock(proc);
		if (!binder_proc_transaction(t, target_proc, target_thread)) {
			binder_inner_proc_lock(proc);
			binder_pop_transaction_ilocked(thread, t);
			binder_inner_proc_unlock(proc);
			goto err_dead_proc_or_thread;
		}
	} else {
		BUG_ON(target_node == NULL);
		BUG_ON(t->buffer->async_transaction != 1);
		binder_enqueue_thread_work(thread, tcomplete);
		if (!binder_proc_transaction(t, target_proc, NULL))
			goto err_dead_proc_or_thread;
	}
	if (target_thread)
		binder_thread_dec_tmpref(target_thread);
	binder_proc_dec_tmpref(target_proc);
	if (target_node)
		binder_dec_node_tmpref(target_node);
	/*
	 * write barrier to synchronize with initialization
	 * of log entry
	 */
	smp_wmb();
	WRITE_ONCE(e->debug_id_done, t_debug_id);
	return;

err_dead_proc_or_thread:
	return_error = BR_DEAD_REPLY;
	return_error_line = __LINE__;
	binder_dequeue_work(proc, tcomplete);
err_translate_failed:
err_bad_object_type:
err_bad_offset:
err_bad_parent:
err_copy_data_failed:
	binder_free_txn_fixups(t);
	trace_binder_transaction_failed_buffer_release(t->buffer);
	binder_transaction_buffer_release(target_proc, t->buffer, offp);
	if (target_node)
		binder_dec_node_tmpref(target_node);
	target_node = NULL;
	t->buffer->transaction = NULL;
	binder_alloc_free_buf(&target_proc->alloc, t->buffer);
err_binder_alloc_buf_failed:
	kfree(tcomplete);
	binder_stats_deleted(BINDER_STAT_TRANSACTION_COMPLETE);
err_alloc_tcomplete_failed:
	kfree(t);
	binder_stats_deleted(BINDER_STAT_TRANSACTION);
err_alloc_t_failed:
err_bad_todo_list:
err_bad_call_stack:
err_empty_call_stack:
err_dead_binder:
err_invalid_target_handle:
	if (target_thread)
		binder_thread_dec_tmpref(target_thread);
	if (target_proc)
		binder_proc_dec_tmpref(target_proc);
	if (target_node) {
		binder_dec_node(target_node, 1, 0);
		binder_dec_node_tmpref(target_node);
	}

	binder_debug(BINDER_DEBUG_FAILED_TRANSACTION,
		     "%d:%d transaction failed %d/%d, size %lld-%lld line %d\n",
		     proc->pid, thread->pid, return_error, return_error_param,
		     (u64)tr->data_size, (u64)tr->offsets_size,
		     return_error_line);

	{
		struct binder_transaction_log_entry *fe;

		e->return_error = return_error;
		e->return_error_param = return_error_param;
		e->return_error_line = return_error_line;
		fe = binder_transaction_log_add(&binder_transaction_log_failed);
		*fe = *e;
		/*
		 * write barrier to synchronize with initialization
		 * of log entry
		 */
		smp_wmb();
		WRITE_ONCE(e->debug_id_done, t_debug_id);
		WRITE_ONCE(fe->debug_id_done, t_debug_id);
	}

	BUG_ON(thread->return_error.cmd != BR_OK);
	if (in_reply_to) {
		thread->return_error.cmd = BR_TRANSACTION_COMPLETE;
		binder_enqueue_thread_work(thread, &thread->return_error.work);
		binder_send_failed_reply(in_reply_to, return_error);
	} else {
		thread->return_error.cmd = return_error;
		binder_enqueue_thread_work(thread, &thread->return_error.work);
	}
}