static void __proc_set_tty(struct tty_struct *tty)
{
	unsigned long flags;

	spin_lock_irqsave(&tty->ctrl_lock, flags);
	/*
	 * The session and fg pgrp references will be non-NULL if
	 * tiocsctty() is stealing the controlling tty
	 */
	put_pid(tty->session);
	put_pid(tty->pgrp);
	tty->pgrp = get_pid(task_pgrp(current));
	spin_unlock_irqrestore(&tty->ctrl_lock, flags);
	tty->session = get_pid(task_session(current));
	if (current->signal->tty) {
		tty_debug(tty, "current tty %s not NULL!!\n",
			  current->signal->tty->name);
		tty_kref_put(current->signal->tty);
	}
	put_pid(current->signal->tty_old_pgrp);
	current->signal->tty = tty_kref_get(tty);
	current->signal->tty_old_pgrp = NULL;
}