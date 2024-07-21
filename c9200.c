static void slcan_write_wakeup(struct tty_struct *tty)
{
	struct slcan *sl = tty->disc_data;

	schedule_work(&sl->tx_work);
}