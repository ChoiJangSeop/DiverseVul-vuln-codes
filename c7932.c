static int toneport_setup(struct usb_line6_toneport *toneport)
{
	u32 *ticks;
	struct usb_line6 *line6 = &toneport->line6;
	struct usb_device *usbdev = line6->usbdev;

	ticks = kmalloc(sizeof(*ticks), GFP_KERNEL);
	if (!ticks)
		return -ENOMEM;

	/* sync time on device with host: */
	/* note: 32-bit timestamps overflow in year 2106 */
	*ticks = (u32)ktime_get_real_seconds();
	line6_write_data(line6, 0x80c6, ticks, 4);
	kfree(ticks);

	/* enable device: */
	toneport_send_cmd(usbdev, 0x0301, 0x0000);

	/* initialize source select: */
	if (toneport_has_source_select(toneport))
		toneport_send_cmd(usbdev,
				  toneport_source_info[toneport->source].code,
				  0x0000);

	if (toneport_has_led(toneport))
		toneport_update_led(toneport);

	schedule_delayed_work(&toneport->pcm_work,
			      msecs_to_jiffies(TONEPORT_PCM_DELAY * 1000));
	return 0;
}