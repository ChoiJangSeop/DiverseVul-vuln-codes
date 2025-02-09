static int snd_seq_ioctl_remove_events(struct snd_seq_client *client,
				       void __user *arg)
{
	struct snd_seq_remove_events info;

	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	/*
	 * Input mostly not implemented XXX.
	 */
	if (info.remove_mode & SNDRV_SEQ_REMOVE_INPUT) {
		/*
		 * No restrictions so for a user client we can clear
		 * the whole fifo
		 */
		if (client->type == USER_CLIENT)
			snd_seq_fifo_clear(client->data.user.fifo);
	}

	if (info.remove_mode & SNDRV_SEQ_REMOVE_OUTPUT)
		snd_seq_queue_remove_cells(client->number, &info);

	return 0;
}