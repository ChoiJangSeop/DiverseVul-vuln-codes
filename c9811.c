void disassociate_ctty(int on_exit)
{
	struct tty_struct *tty;

	if (!current->signal->leader)
		return;

	tty = get_current_tty();
	if (tty) {
		if (on_exit && tty->driver->type != TTY_DRIVER_TYPE_PTY) {
			tty_vhangup_session(tty);
		} else {
			struct pid *tty_pgrp = tty_get_pgrp(tty);
			if (tty_pgrp) {
				kill_pgrp(tty_pgrp, SIGHUP, on_exit);
				if (!on_exit)
					kill_pgrp(tty_pgrp, SIGCONT, on_exit);
				put_pid(tty_pgrp);
			}
		}
		tty_kref_put(tty);

	} else if (on_exit) {
		struct pid *old_pgrp;
		spin_lock_irq(&current->sighand->siglock);
		old_pgrp = current->signal->tty_old_pgrp;
		current->signal->tty_old_pgrp = NULL;
		spin_unlock_irq(&current->sighand->siglock);
		if (old_pgrp) {
			kill_pgrp(old_pgrp, SIGHUP, on_exit);
			kill_pgrp(old_pgrp, SIGCONT, on_exit);
			put_pid(old_pgrp);
		}
		return;
	}

	spin_lock_irq(&current->sighand->siglock);
	put_pid(current->signal->tty_old_pgrp);
	current->signal->tty_old_pgrp = NULL;

	tty = tty_kref_get(current->signal->tty);
	if (tty) {
		unsigned long flags;
		spin_lock_irqsave(&tty->ctrl_lock, flags);
		put_pid(tty->session);
		put_pid(tty->pgrp);
		tty->session = NULL;
		tty->pgrp = NULL;
		spin_unlock_irqrestore(&tty->ctrl_lock, flags);
		tty_kref_put(tty);
	}

	spin_unlock_irq(&current->sighand->siglock);
	/* Now clear signal->tty under the lock */
	read_lock(&tasklist_lock);
	session_clear_tty(task_session(current));
	read_unlock(&tasklist_lock);
}