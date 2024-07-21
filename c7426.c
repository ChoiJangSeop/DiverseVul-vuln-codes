static int binder_thread_write(struct binder_proc *proc,
			struct binder_thread *thread,
			binder_uintptr_t binder_buffer, size_t size,
			binder_size_t *consumed)
{
	uint32_t cmd;
	struct binder_context *context = proc->context;
	void __user *buffer = (void __user *)(uintptr_t)binder_buffer;
	void __user *ptr = buffer + *consumed;
	void __user *end = buffer + size;

	while (ptr < end && thread->return_error.cmd == BR_OK) {
		int ret;

		if (get_user(cmd, (uint32_t __user *)ptr))
			return -EFAULT;
		ptr += sizeof(uint32_t);
		trace_binder_command(cmd);
		if (_IOC_NR(cmd) < ARRAY_SIZE(binder_stats.bc)) {
			atomic_inc(&binder_stats.bc[_IOC_NR(cmd)]);
			atomic_inc(&proc->stats.bc[_IOC_NR(cmd)]);
			atomic_inc(&thread->stats.bc[_IOC_NR(cmd)]);
		}
		switch (cmd) {
		case BC_INCREFS:
		case BC_ACQUIRE:
		case BC_RELEASE:
		case BC_DECREFS: {
			uint32_t target;
			const char *debug_string;
			bool strong = cmd == BC_ACQUIRE || cmd == BC_RELEASE;
			bool increment = cmd == BC_INCREFS || cmd == BC_ACQUIRE;
			struct binder_ref_data rdata;

			if (get_user(target, (uint32_t __user *)ptr))
				return -EFAULT;

			ptr += sizeof(uint32_t);
			ret = -1;
			if (increment && !target) {
				struct binder_node *ctx_mgr_node;
				mutex_lock(&context->context_mgr_node_lock);
				ctx_mgr_node = context->binder_context_mgr_node;
				if (ctx_mgr_node)
					ret = binder_inc_ref_for_node(
							proc, ctx_mgr_node,
							strong, NULL, &rdata);
				mutex_unlock(&context->context_mgr_node_lock);
			}
			if (ret)
				ret = binder_update_ref_for_handle(
						proc, target, increment, strong,
						&rdata);
			if (!ret && rdata.desc != target) {
				binder_user_error("%d:%d tried to acquire reference to desc %d, got %d instead\n",
					proc->pid, thread->pid,
					target, rdata.desc);
			}
			switch (cmd) {
			case BC_INCREFS:
				debug_string = "IncRefs";
				break;
			case BC_ACQUIRE:
				debug_string = "Acquire";
				break;
			case BC_RELEASE:
				debug_string = "Release";
				break;
			case BC_DECREFS:
			default:
				debug_string = "DecRefs";
				break;
			}
			if (ret) {
				binder_user_error("%d:%d %s %d refcount change on invalid ref %d ret %d\n",
					proc->pid, thread->pid, debug_string,
					strong, target, ret);
				break;
			}
			binder_debug(BINDER_DEBUG_USER_REFS,
				     "%d:%d %s ref %d desc %d s %d w %d\n",
				     proc->pid, thread->pid, debug_string,
				     rdata.debug_id, rdata.desc, rdata.strong,
				     rdata.weak);
			break;
		}
		case BC_INCREFS_DONE:
		case BC_ACQUIRE_DONE: {
			binder_uintptr_t node_ptr;
			binder_uintptr_t cookie;
			struct binder_node *node;
			bool free_node;

			if (get_user(node_ptr, (binder_uintptr_t __user *)ptr))
				return -EFAULT;
			ptr += sizeof(binder_uintptr_t);
			if (get_user(cookie, (binder_uintptr_t __user *)ptr))
				return -EFAULT;
			ptr += sizeof(binder_uintptr_t);
			node = binder_get_node(proc, node_ptr);
			if (node == NULL) {
				binder_user_error("%d:%d %s u%016llx no match\n",
					proc->pid, thread->pid,
					cmd == BC_INCREFS_DONE ?
					"BC_INCREFS_DONE" :
					"BC_ACQUIRE_DONE",
					(u64)node_ptr);
				break;
			}
			if (cookie != node->cookie) {
				binder_user_error("%d:%d %s u%016llx node %d cookie mismatch %016llx != %016llx\n",
					proc->pid, thread->pid,
					cmd == BC_INCREFS_DONE ?
					"BC_INCREFS_DONE" : "BC_ACQUIRE_DONE",
					(u64)node_ptr, node->debug_id,
					(u64)cookie, (u64)node->cookie);
				binder_put_node(node);
				break;
			}
			binder_node_inner_lock(node);
			if (cmd == BC_ACQUIRE_DONE) {
				if (node->pending_strong_ref == 0) {
					binder_user_error("%d:%d BC_ACQUIRE_DONE node %d has no pending acquire request\n",
						proc->pid, thread->pid,
						node->debug_id);
					binder_node_inner_unlock(node);
					binder_put_node(node);
					break;
				}
				node->pending_strong_ref = 0;
			} else {
				if (node->pending_weak_ref == 0) {
					binder_user_error("%d:%d BC_INCREFS_DONE node %d has no pending increfs request\n",
						proc->pid, thread->pid,
						node->debug_id);
					binder_node_inner_unlock(node);
					binder_put_node(node);
					break;
				}
				node->pending_weak_ref = 0;
			}
			free_node = binder_dec_node_nilocked(node,
					cmd == BC_ACQUIRE_DONE, 0);
			WARN_ON(free_node);
			binder_debug(BINDER_DEBUG_USER_REFS,
				     "%d:%d %s node %d ls %d lw %d tr %d\n",
				     proc->pid, thread->pid,
				     cmd == BC_INCREFS_DONE ? "BC_INCREFS_DONE" : "BC_ACQUIRE_DONE",
				     node->debug_id, node->local_strong_refs,
				     node->local_weak_refs, node->tmp_refs);
			binder_node_inner_unlock(node);
			binder_put_node(node);
			break;
		}
		case BC_ATTEMPT_ACQUIRE:
			pr_err("BC_ATTEMPT_ACQUIRE not supported\n");
			return -EINVAL;
		case BC_ACQUIRE_RESULT:
			pr_err("BC_ACQUIRE_RESULT not supported\n");
			return -EINVAL;

		case BC_FREE_BUFFER: {
			binder_uintptr_t data_ptr;
			struct binder_buffer *buffer;

			if (get_user(data_ptr, (binder_uintptr_t __user *)ptr))
				return -EFAULT;
			ptr += sizeof(binder_uintptr_t);

			buffer = binder_alloc_prepare_to_free(&proc->alloc,
							      data_ptr);
			if (buffer == NULL) {
				binder_user_error("%d:%d BC_FREE_BUFFER u%016llx no match\n",
					proc->pid, thread->pid, (u64)data_ptr);
				break;
			}
			if (!buffer->allow_user_free) {
				binder_user_error("%d:%d BC_FREE_BUFFER u%016llx matched unreturned buffer\n",
					proc->pid, thread->pid, (u64)data_ptr);
				break;
			}
			binder_debug(BINDER_DEBUG_FREE_BUFFER,
				     "%d:%d BC_FREE_BUFFER u%016llx found buffer %d for %s transaction\n",
				     proc->pid, thread->pid, (u64)data_ptr,
				     buffer->debug_id,
				     buffer->transaction ? "active" : "finished");
			binder_free_buf(proc, buffer);
			break;
		}

		case BC_TRANSACTION_SG:
		case BC_REPLY_SG: {
			struct binder_transaction_data_sg tr;

			if (copy_from_user(&tr, ptr, sizeof(tr)))
				return -EFAULT;
			ptr += sizeof(tr);
			binder_transaction(proc, thread, &tr.transaction_data,
					   cmd == BC_REPLY_SG, tr.buffers_size);
			break;
		}
		case BC_TRANSACTION:
		case BC_REPLY: {
			struct binder_transaction_data tr;

			if (copy_from_user(&tr, ptr, sizeof(tr)))
				return -EFAULT;
			ptr += sizeof(tr);
			binder_transaction(proc, thread, &tr,
					   cmd == BC_REPLY, 0);
			break;
		}

		case BC_REGISTER_LOOPER:
			binder_debug(BINDER_DEBUG_THREADS,
				     "%d:%d BC_REGISTER_LOOPER\n",
				     proc->pid, thread->pid);
			binder_inner_proc_lock(proc);
			if (thread->looper & BINDER_LOOPER_STATE_ENTERED) {
				thread->looper |= BINDER_LOOPER_STATE_INVALID;
				binder_user_error("%d:%d ERROR: BC_REGISTER_LOOPER called after BC_ENTER_LOOPER\n",
					proc->pid, thread->pid);
			} else if (proc->requested_threads == 0) {
				thread->looper |= BINDER_LOOPER_STATE_INVALID;
				binder_user_error("%d:%d ERROR: BC_REGISTER_LOOPER called without request\n",
					proc->pid, thread->pid);
			} else {
				proc->requested_threads--;
				proc->requested_threads_started++;
			}
			thread->looper |= BINDER_LOOPER_STATE_REGISTERED;
			binder_inner_proc_unlock(proc);
			break;
		case BC_ENTER_LOOPER:
			binder_debug(BINDER_DEBUG_THREADS,
				     "%d:%d BC_ENTER_LOOPER\n",
				     proc->pid, thread->pid);
			if (thread->looper & BINDER_LOOPER_STATE_REGISTERED) {
				thread->looper |= BINDER_LOOPER_STATE_INVALID;
				binder_user_error("%d:%d ERROR: BC_ENTER_LOOPER called after BC_REGISTER_LOOPER\n",
					proc->pid, thread->pid);
			}
			thread->looper |= BINDER_LOOPER_STATE_ENTERED;
			break;
		case BC_EXIT_LOOPER:
			binder_debug(BINDER_DEBUG_THREADS,
				     "%d:%d BC_EXIT_LOOPER\n",
				     proc->pid, thread->pid);
			thread->looper |= BINDER_LOOPER_STATE_EXITED;
			break;

		case BC_REQUEST_DEATH_NOTIFICATION:
		case BC_CLEAR_DEATH_NOTIFICATION: {
			uint32_t target;
			binder_uintptr_t cookie;
			struct binder_ref *ref;
			struct binder_ref_death *death = NULL;

			if (get_user(target, (uint32_t __user *)ptr))
				return -EFAULT;
			ptr += sizeof(uint32_t);
			if (get_user(cookie, (binder_uintptr_t __user *)ptr))
				return -EFAULT;
			ptr += sizeof(binder_uintptr_t);
			if (cmd == BC_REQUEST_DEATH_NOTIFICATION) {
				/*
				 * Allocate memory for death notification
				 * before taking lock
				 */
				death = kzalloc(sizeof(*death), GFP_KERNEL);
				if (death == NULL) {
					WARN_ON(thread->return_error.cmd !=
						BR_OK);
					thread->return_error.cmd = BR_ERROR;
					binder_enqueue_thread_work(
						thread,
						&thread->return_error.work);
					binder_debug(
						BINDER_DEBUG_FAILED_TRANSACTION,
						"%d:%d BC_REQUEST_DEATH_NOTIFICATION failed\n",
						proc->pid, thread->pid);
					break;
				}
			}
			binder_proc_lock(proc);
			ref = binder_get_ref_olocked(proc, target, false);
			if (ref == NULL) {
				binder_user_error("%d:%d %s invalid ref %d\n",
					proc->pid, thread->pid,
					cmd == BC_REQUEST_DEATH_NOTIFICATION ?
					"BC_REQUEST_DEATH_NOTIFICATION" :
					"BC_CLEAR_DEATH_NOTIFICATION",
					target);
				binder_proc_unlock(proc);
				kfree(death);
				break;
			}

			binder_debug(BINDER_DEBUG_DEATH_NOTIFICATION,
				     "%d:%d %s %016llx ref %d desc %d s %d w %d for node %d\n",
				     proc->pid, thread->pid,
				     cmd == BC_REQUEST_DEATH_NOTIFICATION ?
				     "BC_REQUEST_DEATH_NOTIFICATION" :
				     "BC_CLEAR_DEATH_NOTIFICATION",
				     (u64)cookie, ref->data.debug_id,
				     ref->data.desc, ref->data.strong,
				     ref->data.weak, ref->node->debug_id);

			binder_node_lock(ref->node);
			if (cmd == BC_REQUEST_DEATH_NOTIFICATION) {
				if (ref->death) {
					binder_user_error("%d:%d BC_REQUEST_DEATH_NOTIFICATION death notification already set\n",
						proc->pid, thread->pid);
					binder_node_unlock(ref->node);
					binder_proc_unlock(proc);
					kfree(death);
					break;
				}
				binder_stats_created(BINDER_STAT_DEATH);
				INIT_LIST_HEAD(&death->work.entry);
				death->cookie = cookie;
				ref->death = death;
				if (ref->node->proc == NULL) {
					ref->death->work.type = BINDER_WORK_DEAD_BINDER;

					binder_inner_proc_lock(proc);
					binder_enqueue_work_ilocked(
						&ref->death->work, &proc->todo);
					binder_wakeup_proc_ilocked(proc);
					binder_inner_proc_unlock(proc);
				}
			} else {
				if (ref->death == NULL) {
					binder_user_error("%d:%d BC_CLEAR_DEATH_NOTIFICATION death notification not active\n",
						proc->pid, thread->pid);
					binder_node_unlock(ref->node);
					binder_proc_unlock(proc);
					break;
				}
				death = ref->death;
				if (death->cookie != cookie) {
					binder_user_error("%d:%d BC_CLEAR_DEATH_NOTIFICATION death notification cookie mismatch %016llx != %016llx\n",
						proc->pid, thread->pid,
						(u64)death->cookie,
						(u64)cookie);
					binder_node_unlock(ref->node);
					binder_proc_unlock(proc);
					break;
				}
				ref->death = NULL;
				binder_inner_proc_lock(proc);
				if (list_empty(&death->work.entry)) {
					death->work.type = BINDER_WORK_CLEAR_DEATH_NOTIFICATION;
					if (thread->looper &
					    (BINDER_LOOPER_STATE_REGISTERED |
					     BINDER_LOOPER_STATE_ENTERED))
						binder_enqueue_thread_work_ilocked(
								thread,
								&death->work);
					else {
						binder_enqueue_work_ilocked(
								&death->work,
								&proc->todo);
						binder_wakeup_proc_ilocked(
								proc);
					}
				} else {
					BUG_ON(death->work.type != BINDER_WORK_DEAD_BINDER);
					death->work.type = BINDER_WORK_DEAD_BINDER_AND_CLEAR;
				}
				binder_inner_proc_unlock(proc);
			}
			binder_node_unlock(ref->node);
			binder_proc_unlock(proc);
		} break;
		case BC_DEAD_BINDER_DONE: {
			struct binder_work *w;
			binder_uintptr_t cookie;
			struct binder_ref_death *death = NULL;

			if (get_user(cookie, (binder_uintptr_t __user *)ptr))
				return -EFAULT;

			ptr += sizeof(cookie);
			binder_inner_proc_lock(proc);
			list_for_each_entry(w, &proc->delivered_death,
					    entry) {
				struct binder_ref_death *tmp_death =
					container_of(w,
						     struct binder_ref_death,
						     work);

				if (tmp_death->cookie == cookie) {
					death = tmp_death;
					break;
				}
			}
			binder_debug(BINDER_DEBUG_DEAD_BINDER,
				     "%d:%d BC_DEAD_BINDER_DONE %016llx found %pK\n",
				     proc->pid, thread->pid, (u64)cookie,
				     death);
			if (death == NULL) {
				binder_user_error("%d:%d BC_DEAD_BINDER_DONE %016llx not found\n",
					proc->pid, thread->pid, (u64)cookie);
				binder_inner_proc_unlock(proc);
				break;
			}
			binder_dequeue_work_ilocked(&death->work);
			if (death->work.type == BINDER_WORK_DEAD_BINDER_AND_CLEAR) {
				death->work.type = BINDER_WORK_CLEAR_DEATH_NOTIFICATION;
				if (thread->looper &
					(BINDER_LOOPER_STATE_REGISTERED |
					 BINDER_LOOPER_STATE_ENTERED))
					binder_enqueue_thread_work_ilocked(
						thread, &death->work);
				else {
					binder_enqueue_work_ilocked(
							&death->work,
							&proc->todo);
					binder_wakeup_proc_ilocked(proc);
				}
			}
			binder_inner_proc_unlock(proc);
		} break;

		default:
			pr_err("%d:%d unknown command %d\n",
			       proc->pid, thread->pid, cmd);
			return -EINVAL;
		}
		*consumed = ptr - buffer;
	}
	return 0;
}