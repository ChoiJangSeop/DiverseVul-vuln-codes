static void mbochs_remove(struct mdev_device *mdev)
{
	struct mdev_state *mdev_state = dev_get_drvdata(&mdev->dev);

	mbochs_used_mbytes -= mdev_state->type->mbytes;
	vfio_unregister_group_dev(&mdev_state->vdev);
	kfree(mdev_state->pages);
	kfree(mdev_state->vconfig);
	kfree(mdev_state);
}