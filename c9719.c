static long snd_rawmidi_kernel_write1(struct snd_rawmidi_substream *substream,
				      const unsigned char __user *userbuf,
				      const unsigned char *kernelbuf,
				      long count)
{
	unsigned long flags;
	long count1, result;
	struct snd_rawmidi_runtime *runtime = substream->runtime;
	unsigned long appl_ptr;

	if (!kernelbuf && !userbuf)
		return -EINVAL;
	if (snd_BUG_ON(!runtime->buffer))
		return -EINVAL;

	result = 0;
	spin_lock_irqsave(&runtime->lock, flags);
	if (substream->append) {
		if ((long)runtime->avail < count) {
			spin_unlock_irqrestore(&runtime->lock, flags);
			return -EAGAIN;
		}
	}
	while (count > 0 && runtime->avail > 0) {
		count1 = runtime->buffer_size - runtime->appl_ptr;
		if (count1 > count)
			count1 = count;
		if (count1 > (long)runtime->avail)
			count1 = runtime->avail;

		/* update runtime->appl_ptr before unlocking for userbuf */
		appl_ptr = runtime->appl_ptr;
		runtime->appl_ptr += count1;
		runtime->appl_ptr %= runtime->buffer_size;
		runtime->avail -= count1;

		if (kernelbuf)
			memcpy(runtime->buffer + appl_ptr,
			       kernelbuf + result, count1);
		else if (userbuf) {
			spin_unlock_irqrestore(&runtime->lock, flags);
			if (copy_from_user(runtime->buffer + appl_ptr,
					   userbuf + result, count1)) {
				spin_lock_irqsave(&runtime->lock, flags);
				result = result > 0 ? result : -EFAULT;
				goto __end;
			}
			spin_lock_irqsave(&runtime->lock, flags);
		}
		result += count1;
		count -= count1;
	}
      __end:
	count1 = runtime->avail < runtime->buffer_size;
	spin_unlock_irqrestore(&runtime->lock, flags);
	if (count1)
		snd_rawmidi_output_trigger(substream, 1);
	return result;
}