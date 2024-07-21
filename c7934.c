static void line6_toneport_disconnect(struct usb_line6 *line6)
{
	struct usb_line6_toneport *toneport =
		(struct usb_line6_toneport *)line6;

	cancel_delayed_work_sync(&toneport->pcm_work);

	if (toneport_has_led(toneport))
		toneport_remove_leds(toneport);
}