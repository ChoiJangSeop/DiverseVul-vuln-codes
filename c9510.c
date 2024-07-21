static ssize_t do_rbd_remove(struct bus_type *bus,
			     const char *buf,
			     size_t count)
{
	struct rbd_device *rbd_dev = NULL;
	struct list_head *tmp;
	int dev_id;
	char opt_buf[6];
	bool force = false;
	int ret;

	dev_id = -1;
	opt_buf[0] = '\0';
	sscanf(buf, "%d %5s", &dev_id, opt_buf);
	if (dev_id < 0) {
		pr_err("dev_id out of range\n");
		return -EINVAL;
	}
	if (opt_buf[0] != '\0') {
		if (!strcmp(opt_buf, "force")) {
			force = true;
		} else {
			pr_err("bad remove option at '%s'\n", opt_buf);
			return -EINVAL;
		}
	}

	ret = -ENOENT;
	spin_lock(&rbd_dev_list_lock);
	list_for_each(tmp, &rbd_dev_list) {
		rbd_dev = list_entry(tmp, struct rbd_device, node);
		if (rbd_dev->dev_id == dev_id) {
			ret = 0;
			break;
		}
	}
	if (!ret) {
		spin_lock_irq(&rbd_dev->lock);
		if (rbd_dev->open_count && !force)
			ret = -EBUSY;
		else if (test_and_set_bit(RBD_DEV_FLAG_REMOVING,
					  &rbd_dev->flags))
			ret = -EINPROGRESS;
		spin_unlock_irq(&rbd_dev->lock);
	}
	spin_unlock(&rbd_dev_list_lock);
	if (ret)
		return ret;

	if (force) {
		/*
		 * Prevent new IO from being queued and wait for existing
		 * IO to complete/fail.
		 */
		blk_mq_freeze_queue(rbd_dev->disk->queue);
		blk_set_queue_dying(rbd_dev->disk->queue);
	}

	del_gendisk(rbd_dev->disk);
	spin_lock(&rbd_dev_list_lock);
	list_del_init(&rbd_dev->node);
	spin_unlock(&rbd_dev_list_lock);
	device_del(&rbd_dev->dev);

	rbd_dev_image_unlock(rbd_dev);
	rbd_dev_device_release(rbd_dev);
	rbd_dev_image_release(rbd_dev);
	rbd_dev_destroy(rbd_dev);
	return count;
}