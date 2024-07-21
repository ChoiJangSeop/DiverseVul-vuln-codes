static int mbochs_probe(struct mdev_device *mdev)
{
	const struct mbochs_type *type =
		&mbochs_types[mdev_get_type_group_id(mdev)];
	struct device *dev = mdev_dev(mdev);
	struct mdev_state *mdev_state;
	int ret = -ENOMEM;

	if (type->mbytes + mbochs_used_mbytes > max_mbytes)
		return -ENOMEM;

	mdev_state = kzalloc(sizeof(struct mdev_state), GFP_KERNEL);
	if (mdev_state == NULL)
		return -ENOMEM;
	vfio_init_group_dev(&mdev_state->vdev, &mdev->dev, &mbochs_dev_ops);

	mdev_state->vconfig = kzalloc(MBOCHS_CONFIG_SPACE_SIZE, GFP_KERNEL);
	if (mdev_state->vconfig == NULL)
		goto err_mem;

	mdev_state->memsize = type->mbytes * 1024 * 1024;
	mdev_state->pagecount = mdev_state->memsize >> PAGE_SHIFT;
	mdev_state->pages = kcalloc(mdev_state->pagecount,
				    sizeof(struct page *),
				    GFP_KERNEL);
	if (!mdev_state->pages)
		goto err_mem;

	dev_info(dev, "%s: %s, %d MB, %ld pages\n", __func__,
		 type->name, type->mbytes, mdev_state->pagecount);

	mutex_init(&mdev_state->ops_lock);
	mdev_state->mdev = mdev;
	INIT_LIST_HEAD(&mdev_state->dmabufs);
	mdev_state->next_id = 1;

	mdev_state->type = type;
	mdev_state->edid_regs.max_xres = type->max_x;
	mdev_state->edid_regs.max_yres = type->max_y;
	mdev_state->edid_regs.edid_offset = MBOCHS_EDID_BLOB_OFFSET;
	mdev_state->edid_regs.edid_max_size = sizeof(mdev_state->edid_blob);
	mbochs_create_config_space(mdev_state);
	mbochs_reset(mdev_state);

	mbochs_used_mbytes += type->mbytes;

	ret = vfio_register_group_dev(&mdev_state->vdev);
	if (ret)
		goto err_mem;
	dev_set_drvdata(&mdev->dev, mdev_state);
	return 0;

err_mem:
	kfree(mdev_state->vconfig);
	kfree(mdev_state);
	return ret;
}