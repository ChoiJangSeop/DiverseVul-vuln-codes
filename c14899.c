static ssize_t __cgroup1_procs_write(struct kernfs_open_file *of,
				     char *buf, size_t nbytes, loff_t off,
				     bool threadgroup)
{
	struct cgroup *cgrp;
	struct task_struct *task;
	const struct cred *cred, *tcred;
	ssize_t ret;
	bool locked;

	cgrp = cgroup_kn_lock_live(of->kn, false);
	if (!cgrp)
		return -ENODEV;

	task = cgroup_procs_write_start(buf, threadgroup, &locked);
	ret = PTR_ERR_OR_ZERO(task);
	if (ret)
		goto out_unlock;

	/*
	 * Even if we're attaching all tasks in the thread group, we only
	 * need to check permissions on one of them.
	 */
	cred = current_cred();
	tcred = get_task_cred(task);
	if (!uid_eq(cred->euid, GLOBAL_ROOT_UID) &&
	    !uid_eq(cred->euid, tcred->uid) &&
	    !uid_eq(cred->euid, tcred->suid))
		ret = -EACCES;
	put_cred(tcred);
	if (ret)
		goto out_finish;

	ret = cgroup_attach_task(cgrp, task, threadgroup);

out_finish:
	cgroup_procs_write_finish(task, locked);
out_unlock:
	cgroup_kn_unlock(of->kn);

	return ret ?: nbytes;
}