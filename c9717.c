static int resize_runtime_buffer(struct snd_rawmidi_runtime *runtime,
				 struct snd_rawmidi_params *params,
				 bool is_input)
{
	char *newbuf, *oldbuf;

	if (params->buffer_size < 32 || params->buffer_size > 1024L * 1024L)
		return -EINVAL;
	if (params->avail_min < 1 || params->avail_min > params->buffer_size)
		return -EINVAL;
	if (params->buffer_size != runtime->buffer_size) {
		newbuf = kvzalloc(params->buffer_size, GFP_KERNEL);
		if (!newbuf)
			return -ENOMEM;
		spin_lock_irq(&runtime->lock);
		oldbuf = runtime->buffer;
		runtime->buffer = newbuf;
		runtime->buffer_size = params->buffer_size;
		__reset_runtime_ptrs(runtime, is_input);
		spin_unlock_irq(&runtime->lock);
		kvfree(oldbuf);
	}
	runtime->avail_min = params->avail_min;
	return 0;
}