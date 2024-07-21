static int check_unsafe_exec(struct linux_binprm *bprm)
{
	struct task_struct *p = current, *t;
	unsigned n_fs;
	int res = 0;

	if (p->ptrace) {
		if (p->ptrace & PT_PTRACE_CAP)
			bprm->unsafe |= LSM_UNSAFE_PTRACE_CAP;
		else
			bprm->unsafe |= LSM_UNSAFE_PTRACE;
	}

	n_fs = 1;
	spin_lock(&p->fs->lock);
	rcu_read_lock();
	for (t = next_thread(p); t != p; t = next_thread(t)) {
		if (t->fs == p->fs)
			n_fs++;
	}
	rcu_read_unlock();

	if (p->fs->users > n_fs) {
		bprm->unsafe |= LSM_UNSAFE_SHARE;
	} else {
		res = -EAGAIN;
		if (!p->fs->in_exec) {
			p->fs->in_exec = 1;
			res = 1;
		}
	}
	spin_unlock(&p->fs->lock);

	return res;
}