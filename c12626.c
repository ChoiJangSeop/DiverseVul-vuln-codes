int scsi_cmd_blk_ioctl(struct block_device *bd, fmode_t mode,
		       unsigned int cmd, void __user *arg)
{
	return scsi_cmd_ioctl(bd->bd_disk->queue, bd->bd_disk, mode, cmd, arg);
}