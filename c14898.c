static ssize_t __cgroup_procs_write(struct kernfs_open_file *of, char *buf,
				    bool threadgroup)
{
	struct cgroup *src_cgrp, *dst_cgrp;
	struct task_struct *task;
	ssize_t ret;
	bool locked;

	dst_cgrp = cgroup_kn_lock_live(of->kn, false);
	if (!dst_cgrp)
		return -ENODEV;

	task = cgroup_procs_write_start(buf, threadgroup, &locked);
	ret = PTR_ERR_OR_ZERO(task);
	if (ret)
		goto out_unlock;

	/* find the source cgroup */
	spin_lock_irq(&css_set_lock);
	src_cgrp = task_cgroup_from_root(task, &cgrp_dfl_root);
	spin_unlock_irq(&css_set_lock);

	/* process and thread migrations follow same delegation rule */
	ret = cgroup_attach_permissions(src_cgrp, dst_cgrp,
					of->file->f_path.dentry->d_sb, threadgroup);
	if (ret)
		goto out_finish;

	ret = cgroup_attach_task(dst_cgrp, task, threadgroup);

out_finish:
	cgroup_procs_write_finish(task, locked);
out_unlock:
	cgroup_kn_unlock(of->kn);

	return ret;
}