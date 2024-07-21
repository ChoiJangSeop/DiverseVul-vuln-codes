static int snd_pcm_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime;
	int err, usecs;
	unsigned int bits;
	snd_pcm_uframes_t frames;

	if (PCM_RUNTIME_CHECK(substream))
		return -ENXIO;
	runtime = substream->runtime;
	snd_pcm_stream_lock_irq(substream);
	switch (runtime->status->state) {
	case SNDRV_PCM_STATE_OPEN:
	case SNDRV_PCM_STATE_SETUP:
	case SNDRV_PCM_STATE_PREPARED:
		break;
	default:
		snd_pcm_stream_unlock_irq(substream);
		return -EBADFD;
	}
	snd_pcm_stream_unlock_irq(substream);
#if IS_ENABLED(CONFIG_SND_PCM_OSS)
	if (!substream->oss.oss)
#endif
		if (atomic_read(&substream->mmap_count))
			return -EBADFD;

	snd_pcm_sync_stop(substream, true);

	params->rmask = ~0U;
	err = snd_pcm_hw_refine(substream, params);
	if (err < 0)
		goto _error;

	err = snd_pcm_hw_params_choose(substream, params);
	if (err < 0)
		goto _error;

	err = fixup_unreferenced_params(substream, params);
	if (err < 0)
		goto _error;

	if (substream->managed_buffer_alloc) {
		err = snd_pcm_lib_malloc_pages(substream,
					       params_buffer_bytes(params));
		if (err < 0)
			goto _error;
		runtime->buffer_changed = err > 0;
	}

	if (substream->ops->hw_params != NULL) {
		err = substream->ops->hw_params(substream, params);
		if (err < 0)
			goto _error;
	}

	runtime->access = params_access(params);
	runtime->format = params_format(params);
	runtime->subformat = params_subformat(params);
	runtime->channels = params_channels(params);
	runtime->rate = params_rate(params);
	runtime->period_size = params_period_size(params);
	runtime->periods = params_periods(params);
	runtime->buffer_size = params_buffer_size(params);
	runtime->info = params->info;
	runtime->rate_num = params->rate_num;
	runtime->rate_den = params->rate_den;
	runtime->no_period_wakeup =
			(params->info & SNDRV_PCM_INFO_NO_PERIOD_WAKEUP) &&
			(params->flags & SNDRV_PCM_HW_PARAMS_NO_PERIOD_WAKEUP);

	bits = snd_pcm_format_physical_width(runtime->format);
	runtime->sample_bits = bits;
	bits *= runtime->channels;
	runtime->frame_bits = bits;
	frames = 1;
	while (bits % 8 != 0) {
		bits *= 2;
		frames *= 2;
	}
	runtime->byte_align = bits / 8;
	runtime->min_align = frames;

	/* Default sw params */
	runtime->tstamp_mode = SNDRV_PCM_TSTAMP_NONE;
	runtime->period_step = 1;
	runtime->control->avail_min = runtime->period_size;
	runtime->start_threshold = 1;
	runtime->stop_threshold = runtime->buffer_size;
	runtime->silence_threshold = 0;
	runtime->silence_size = 0;
	runtime->boundary = runtime->buffer_size;
	while (runtime->boundary * 2 <= LONG_MAX - runtime->buffer_size)
		runtime->boundary *= 2;

	/* clear the buffer for avoiding possible kernel info leaks */
	if (runtime->dma_area && !substream->ops->copy_user) {
		size_t size = runtime->dma_bytes;

		if (runtime->info & SNDRV_PCM_INFO_MMAP)
			size = PAGE_ALIGN(size);
		memset(runtime->dma_area, 0, size);
	}

	snd_pcm_timer_resolution_change(substream);
	snd_pcm_set_state(substream, SNDRV_PCM_STATE_SETUP);

	if (cpu_latency_qos_request_active(&substream->latency_pm_qos_req))
		cpu_latency_qos_remove_request(&substream->latency_pm_qos_req);
	usecs = period_to_usecs(runtime);
	if (usecs >= 0)
		cpu_latency_qos_add_request(&substream->latency_pm_qos_req,
					    usecs);
	return 0;
 _error:
	/* hardware might be unusable from this time,
	   so we force application to retry to set
	   the correct hardware parameter settings */
	snd_pcm_set_state(substream, SNDRV_PCM_STATE_OPEN);
	if (substream->ops->hw_free != NULL)
		substream->ops->hw_free(substream);
	if (substream->managed_buffer_alloc)
		snd_pcm_lib_free_pages(substream);
	return err;
}