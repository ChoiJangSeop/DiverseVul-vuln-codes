static ssize_t snd_timer_user_read(struct file *file, char __user *buffer,
				   size_t count, loff_t *offset)
{
	struct snd_timer_user *tu;
	long result = 0, unit;
	int err = 0;

	tu = file->private_data;
	unit = tu->tread ? sizeof(struct snd_timer_tread) : sizeof(struct snd_timer_read);
	spin_lock_irq(&tu->qlock);
	while ((long)count - result >= unit) {
		while (!tu->qused) {
			wait_queue_t wait;

			if ((file->f_flags & O_NONBLOCK) != 0 || result > 0) {
				err = -EAGAIN;
				break;
			}

			set_current_state(TASK_INTERRUPTIBLE);
			init_waitqueue_entry(&wait, current);
			add_wait_queue(&tu->qchange_sleep, &wait);

			spin_unlock_irq(&tu->qlock);
			schedule();
			spin_lock_irq(&tu->qlock);

			remove_wait_queue(&tu->qchange_sleep, &wait);

			if (tu->disconnected) {
				err = -ENODEV;
				break;
			}
			if (signal_pending(current)) {
				err = -ERESTARTSYS;
				break;
			}
		}

		spin_unlock_irq(&tu->qlock);
		if (err < 0)
			goto _error;

		if (tu->tread) {
			if (copy_to_user(buffer, &tu->tqueue[tu->qhead++],
					 sizeof(struct snd_timer_tread))) {
				err = -EFAULT;
				goto _error;
			}
		} else {
			if (copy_to_user(buffer, &tu->queue[tu->qhead++],
					 sizeof(struct snd_timer_read))) {
				err = -EFAULT;
				goto _error;
			}
		}

		tu->qhead %= tu->queue_size;

		result += unit;
		buffer += unit;

		spin_lock_irq(&tu->qlock);
		tu->qused--;
	}
	spin_unlock_irq(&tu->qlock);
 _error:
	return result > 0 ? result : err;
}