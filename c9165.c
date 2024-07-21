
static int netdev_queue_add_kobject(struct net_device *dev, int index)
{
	struct netdev_queue *queue = dev->_tx + index;
	struct kobject *kobj = &queue->kobj;
	int error = 0;

	kobj->kset = dev->queues_kset;
	error = kobject_init_and_add(kobj, &netdev_queue_ktype, NULL,
				     "tx-%u", index);
	if (error)
		return error;

#ifdef CONFIG_BQL
	error = sysfs_create_group(kobj, &dql_group);
	if (error) {
		kobject_put(kobj);
		return error;
	}
#endif

	kobject_uevent(kobj, KOBJ_ADD);
	dev_hold(queue->dev);

	return 0;