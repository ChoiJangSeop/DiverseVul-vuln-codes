void uwbd_start(struct uwb_rc *rc)
{
	rc->uwbd.task = kthread_run(uwbd, rc, "uwbd");
	if (rc->uwbd.task == NULL)
		printk(KERN_ERR "UWB: Cannot start management daemon; "
		       "UWB won't work\n");
	else
		rc->uwbd.pid = rc->uwbd.task->pid;
}