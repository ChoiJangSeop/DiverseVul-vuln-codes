static void sunkbd_enable(struct sunkbd *sunkbd, bool enable)
{
	serio_pause_rx(sunkbd->serio);
	sunkbd->enabled = enable;
	serio_continue_rx(sunkbd->serio);
}