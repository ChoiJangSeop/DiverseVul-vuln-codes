static int flakey_ioctl(struct dm_target *ti, unsigned int cmd, unsigned long arg)
{
	struct flakey_c *fc = ti->private;

	return __blkdev_driver_ioctl(fc->dev->bdev, fc->dev->mode, cmd, arg);
}