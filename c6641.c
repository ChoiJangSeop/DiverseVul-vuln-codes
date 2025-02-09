static ssize_t hid_debug_events_read(struct file *file, char __user *buffer,
		size_t count, loff_t *ppos)
{
	struct hid_debug_list *list = file->private_data;
	int ret = 0, len;
	DECLARE_WAITQUEUE(wait, current);

	mutex_lock(&list->read_mutex);
	while (ret == 0) {
		if (list->head == list->tail) {
			add_wait_queue(&list->hdev->debug_wait, &wait);
			set_current_state(TASK_INTERRUPTIBLE);

			while (list->head == list->tail) {
				if (file->f_flags & O_NONBLOCK) {
					ret = -EAGAIN;
					break;
				}
				if (signal_pending(current)) {
					ret = -ERESTARTSYS;
					break;
				}

				if (!list->hdev || !list->hdev->debug) {
					ret = -EIO;
					set_current_state(TASK_RUNNING);
					goto out;
				}

				/* allow O_NONBLOCK from other threads */
				mutex_unlock(&list->read_mutex);
				schedule();
				mutex_lock(&list->read_mutex);
				set_current_state(TASK_INTERRUPTIBLE);
			}

			set_current_state(TASK_RUNNING);
			remove_wait_queue(&list->hdev->debug_wait, &wait);
		}

		if (ret)
			goto out;

		/* pass the ringbuffer contents to userspace */
copy_rest:
		if (list->tail == list->head)
			goto out;
		if (list->tail > list->head) {
			len = list->tail - list->head;

			if (copy_to_user(buffer + ret, &list->hid_debug_buf[list->head], len)) {
				ret = -EFAULT;
				goto out;
			}
			ret += len;
			list->head += len;
		} else {
			len = HID_DEBUG_BUFSIZE - list->head;

			if (copy_to_user(buffer, &list->hid_debug_buf[list->head], len)) {
				ret = -EFAULT;
				goto out;
			}
			list->head = 0;
			ret += len;
			goto copy_rest;
		}

	}
out:
	mutex_unlock(&list->read_mutex);
	return ret;
}