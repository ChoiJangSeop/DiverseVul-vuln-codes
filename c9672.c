static int spk_ttyio_ldisc_open(struct tty_struct *tty)
{
	struct spk_ldisc_data *ldisc_data;

	if (!tty->ops->write)
		return -EOPNOTSUPP;
	speakup_tty = tty;

	ldisc_data = kmalloc(sizeof(*ldisc_data), GFP_KERNEL);
	if (!ldisc_data)
		return -ENOMEM;

	init_completion(&ldisc_data->completion);
	ldisc_data->buf_free = true;
	speakup_tty->disc_data = ldisc_data;

	return 0;
}