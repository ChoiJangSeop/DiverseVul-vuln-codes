delete_port(struct seq_oss_devinfo *dp)
{
	if (dp->port < 0)
		return 0;

	debug_printk(("delete_port %i\n", dp->port));
	return snd_seq_event_port_detach(dp->cseq, dp->port);
}