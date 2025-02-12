int sequencer_write(int dev, struct file *file, const char __user *buf, int count)
{
	unsigned char event_rec[EV_SZ], ev_code;
	int p = 0, c, ev_size;
	int mode = translate_mode(file);

	dev = dev >> 4;

	DEB(printk("sequencer_write(dev=%d, count=%d)\n", dev, count));

	if (mode == OPEN_READ)
		return -EIO;

	c = count;

	while (c >= 4)
	{
		if (copy_from_user((char *) event_rec, &(buf)[p], 4))
			goto out;
		ev_code = event_rec[0];

		if (ev_code == SEQ_FULLSIZE)
		{
			int err, fmt;

			dev = *(unsigned short *) &event_rec[2];
			if (dev < 0 || dev >= max_synthdev || synth_devs[dev] == NULL)
				return -ENXIO;

			if (!(synth_open_mask & (1 << dev)))
				return -ENXIO;

			fmt = (*(short *) &event_rec[0]) & 0xffff;
			err = synth_devs[dev]->load_patch(dev, fmt, buf, p + 4, c, 0);
			if (err < 0)
				return err;

			return err;
		}
		if (ev_code >= 128)
		{
			if (seq_mode == SEQ_2 && ev_code == SEQ_EXTENDED)
			{
				printk(KERN_WARNING "Sequencer: Invalid level 2 event %x\n", ev_code);
				return -EINVAL;
			}
			ev_size = 8;

			if (c < ev_size)
			{
				if (!seq_playing)
					seq_startplay();
				return count - c;
			}
			if (copy_from_user((char *)&event_rec[4],
					   &(buf)[p + 4], 4))
				goto out;

		}
		else
		{
			if (seq_mode == SEQ_2)
			{
				printk(KERN_WARNING "Sequencer: 4 byte event in level 2 mode\n");
				return -EINVAL;
			}
			ev_size = 4;

			if (event_rec[0] != SEQ_MIDIPUTC)
				obsolete_api_used = 1;
		}

		if (event_rec[0] == SEQ_MIDIPUTC)
		{
			if (!midi_opened[event_rec[2]])
			{
				int err, mode;
				int dev = event_rec[2];

				if (dev >= max_mididev || midi_devs[dev]==NULL)
				{
					/*printk("Sequencer Error: Nonexistent MIDI device %d\n", dev);*/
					return -ENXIO;
				}
				mode = translate_mode(file);

				if ((err = midi_devs[dev]->open(dev, mode,
								sequencer_midi_input, sequencer_midi_output)) < 0)
				{
					seq_reset();
					printk(KERN_WARNING "Sequencer Error: Unable to open Midi #%d\n", dev);
					return err;
				}
				midi_opened[dev] = 1;
			}
		}
		if (!seq_queue(event_rec, (file->f_flags & (O_NONBLOCK) ? 1 : 0)))
		{
			int processed = count - c;

			if (!seq_playing)
				seq_startplay();

			if (!processed && (file->f_flags & O_NONBLOCK))
				return -EAGAIN;
			else
				return processed;
		}
		p += ev_size;
		c -= ev_size;
	}

	if (!seq_playing)
		seq_startplay();
out:
	return count;
}