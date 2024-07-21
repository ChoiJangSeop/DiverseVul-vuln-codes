static void mctp_serial_close(struct tty_struct *tty)
{
	struct mctp_serial *dev = tty->disc_data;
	int idx = dev->idx;

	unregister_netdev(dev->netdev);
	cancel_work_sync(&dev->tx_work);
	ida_free(&mctp_serial_ida, idx);
}