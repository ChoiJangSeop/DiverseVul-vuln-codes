int snd_pcm_attach_substream(struct snd_pcm *pcm, int stream,
			     struct file *file,
			     struct snd_pcm_substream **rsubstream)
{
	struct snd_pcm_str * pstr;
	struct snd_pcm_substream *substream;
	struct snd_pcm_runtime *runtime;
	struct snd_card *card;
	int prefer_subdevice;
	size_t size;

	if (snd_BUG_ON(!pcm || !rsubstream))
		return -ENXIO;
	if (snd_BUG_ON(stream != SNDRV_PCM_STREAM_PLAYBACK &&
		       stream != SNDRV_PCM_STREAM_CAPTURE))
		return -EINVAL;
	*rsubstream = NULL;
	pstr = &pcm->streams[stream];
	if (pstr->substream == NULL || pstr->substream_count == 0)
		return -ENODEV;

	card = pcm->card;
	prefer_subdevice = snd_ctl_get_preferred_subdevice(card, SND_CTL_SUBDEV_PCM);

	if (pcm->info_flags & SNDRV_PCM_INFO_HALF_DUPLEX) {
		int opposite = !stream;

		for (substream = pcm->streams[opposite].substream; substream;
		     substream = substream->next) {
			if (SUBSTREAM_BUSY(substream))
				return -EAGAIN;
		}
	}

	if (file->f_flags & O_APPEND) {
		if (prefer_subdevice < 0) {
			if (pstr->substream_count > 1)
				return -EINVAL; /* must be unique */
			substream = pstr->substream;
		} else {
			for (substream = pstr->substream; substream;
			     substream = substream->next)
				if (substream->number == prefer_subdevice)
					break;
		}
		if (! substream)
			return -ENODEV;
		if (! SUBSTREAM_BUSY(substream))
			return -EBADFD;
		substream->ref_count++;
		*rsubstream = substream;
		return 0;
	}

	for (substream = pstr->substream; substream; substream = substream->next) {
		if (!SUBSTREAM_BUSY(substream) &&
		    (prefer_subdevice == -1 ||
		     substream->number == prefer_subdevice))
			break;
	}
	if (substream == NULL)
		return -EAGAIN;

	runtime = kzalloc(sizeof(*runtime), GFP_KERNEL);
	if (runtime == NULL)
		return -ENOMEM;

	size = PAGE_ALIGN(sizeof(struct snd_pcm_mmap_status));
	runtime->status = alloc_pages_exact(size, GFP_KERNEL);
	if (runtime->status == NULL) {
		kfree(runtime);
		return -ENOMEM;
	}
	memset(runtime->status, 0, size);

	size = PAGE_ALIGN(sizeof(struct snd_pcm_mmap_control));
	runtime->control = alloc_pages_exact(size, GFP_KERNEL);
	if (runtime->control == NULL) {
		free_pages_exact(runtime->status,
			       PAGE_ALIGN(sizeof(struct snd_pcm_mmap_status)));
		kfree(runtime);
		return -ENOMEM;
	}
	memset(runtime->control, 0, size);

	init_waitqueue_head(&runtime->sleep);
	init_waitqueue_head(&runtime->tsleep);

	runtime->status->state = SNDRV_PCM_STATE_OPEN;

	substream->runtime = runtime;
	substream->private_data = pcm->private_data;
	substream->ref_count = 1;
	substream->f_flags = file->f_flags;
	substream->pid = get_pid(task_pid(current));
	pstr->substream_opened++;
	*rsubstream = substream;
	return 0;
}