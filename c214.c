static int rtc_dev_fasync(int fd, struct file *file, int on)
{
	struct rtc_device *rtc = file->private_data;
	return fasync_helper(fd, file, on, &rtc->async_queue);
}