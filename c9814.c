static int tiocspgrp(struct tty_struct *tty, struct tty_struct *real_tty, pid_t __user *p)
{
	struct pid *pgrp;
	pid_t pgrp_nr;
	int retval = tty_check_change(real_tty);

	if (retval == -EIO)
		return -ENOTTY;
	if (retval)
		return retval;
	if (!current->signal->tty ||
	    (current->signal->tty != real_tty) ||
	    (real_tty->session != task_session(current)))
		return -ENOTTY;
	if (get_user(pgrp_nr, p))
		return -EFAULT;
	if (pgrp_nr < 0)
		return -EINVAL;
	rcu_read_lock();
	pgrp = find_vpid(pgrp_nr);
	retval = -ESRCH;
	if (!pgrp)
		goto out_unlock;
	retval = -EPERM;
	if (session_of_pgrp(pgrp) != task_session(current))
		goto out_unlock;
	retval = 0;
	spin_lock_irq(&tty->ctrl_lock);
	put_pid(real_tty->pgrp);
	real_tty->pgrp = get_pid(pgrp);
	spin_unlock_irq(&tty->ctrl_lock);
out_unlock:
	rcu_read_unlock();
	return retval;
}