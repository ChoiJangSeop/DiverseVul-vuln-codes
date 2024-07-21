static int rtc_dev_release(struct inode *inode, struct file *file)
{
	struct rtc_device *rtc = file->private_data;

#ifdef CONFIG_RTC_INTF_DEV_UIE_EMUL
	clear_uie(rtc);
#endif
	rtc_irq_set_state(rtc, NULL, 0);

	if (rtc->ops->release)
		rtc->ops->release(rtc->dev.parent);

	clear_bit_unlock(RTC_DEV_BUSY, &rtc->flags);
	return 0;
}