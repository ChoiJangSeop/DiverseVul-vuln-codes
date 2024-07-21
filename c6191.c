int snd_timer_stop(struct snd_timer_instance *timeri)
{
	struct snd_timer *timer;
	unsigned long flags;
	int err;

	err = _snd_timer_stop(timeri, 0, SNDRV_TIMER_EVENT_STOP);
	if (err < 0)
		return err;
	timer = timeri->timer;
	if (!timer)
		return -EINVAL;
	spin_lock_irqsave(&timer->lock, flags);
	timeri->cticks = timeri->ticks;
	timeri->pticks = 0;
	spin_unlock_irqrestore(&timer->lock, flags);
	return 0;
}