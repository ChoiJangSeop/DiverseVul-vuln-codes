static void toneport_start_pcm(struct work_struct *work)
{
	struct usb_line6_toneport *toneport =
		container_of(work, struct usb_line6_toneport, pcm_work.work);
	struct usb_line6 *line6 = &toneport->line6;

	line6_pcm_acquire(line6->line6pcm, LINE6_STREAM_MONITOR, true);
}