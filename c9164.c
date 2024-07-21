
static int rx_queue_add_kobject(struct net_device *dev, int index)
{
	struct netdev_rx_queue *queue = dev->_rx + index;
	struct kobject *kobj = &queue->kobj;
	int error = 0;

	kobj->kset = dev->queues_kset;
	error = kobject_init_and_add(kobj, &rx_queue_ktype, NULL,
				     "rx-%u", index);
	if (error)
		return error;

	if (dev->sysfs_rx_queue_group) {
		error = sysfs_create_group(kobj, dev->sysfs_rx_queue_group);
		if (error) {
			kobject_put(kobj);
			return error;
		}
	}

	kobject_uevent(kobj, KOBJ_ADD);
	dev_hold(queue->dev);

	return error;