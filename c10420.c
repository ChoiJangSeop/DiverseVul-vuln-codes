void elevator_exit(struct request_queue *q, struct elevator_queue *e)
{
	mutex_lock(&e->sysfs_lock);
	if (e->type->ops.exit_sched)
		blk_mq_exit_sched(q, e);
	mutex_unlock(&e->sysfs_lock);

	kobject_put(&e->kobj);
}